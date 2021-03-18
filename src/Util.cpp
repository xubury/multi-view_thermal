#include <Image.hpp>
#include "Util.hpp"
#include <algorithm>
#include "mve/image_tools.h"
#include "util/file_system.h"
#include "util/timer.h"
#include "mve/mesh_info.h"
#include "mve/mesh_io.h"
#include "mve/mesh_io_ply.h"
#include "thread_pool.h"
#include "stereo_view.h"
#include "depth_optimizer.h"
#include "mesh_generator.h"
#include "view_selection.h"
#include "sgm_stereo.h"

namespace Util {

glm::mat4 MveToGLMatrix(const glm::mat4 &matrix) {
    glm::mat4 out;
    out = glm::transpose(matrix);
    out[1] = -out[1];
    out[2] = -out[2];
    return out;
}

void GaussFilter(const mve::FloatImage::ConstPtr &img,
                 mve::FloatImage::Ptr &out,
                 int filter_range,
                 float sigma) {
    assert(img->channels() == 1);

    out = mve::FloatImage::create(img->width() - filter_range * 2, img->height() - filter_range * 2, 1);

    int k_size = filter_range * 2 + 1;
    std::vector<double> gauss_template(k_size);
    double sum = 0;
    for (int a = 0; a < k_size; a++) {
        int x = a - filter_range;
        double m = std::exp((-1.0f * x * x) / (2.f * sigma * sigma));
        gauss_template[a] = m;
        sum += m;
    }
    std::transform(gauss_template.begin(),
                   gauss_template.end(),
                   gauss_template.begin(),
                   [&sum](auto &c) { return c / sum; });
#pragma omp parallel for schedule(dynamic, 1)
    for (int y = filter_range; y < img->height() - filter_range; ++y) {
#pragma omp parallel for schedule(dynamic, 1)
        for (int x = filter_range; x < img->width() - filter_range; ++x) {
            double res = 0;
            for (int a = -filter_range; a <= filter_range; a++) {
                res += gauss_template.at(a + filter_range) * img->at(x, y + a, 0);
            }
            out->at(x - filter_range, y - filter_range, 0) = res;
        }
    }
#pragma omp parallel for schedule(dynamic, 1)
    for (int y = filter_range; y < img->height() - filter_range; ++y) {
#pragma omp parallel for schedule(dynamic, 1)
        for (int x = filter_range; x < img->width() - filter_range; ++x) {
            double res = 0;
            for (int a = -filter_range; a <= filter_range; a++) {
                res += gauss_template.at(a + filter_range) * out->at(x - filter_range + a, y - filter_range, 0);
            }
            out->at(x - filter_range, y - filter_range, 0) = res;
        }
    }
}

mve::FloatImage::Ptr ComputeIntegralImg(const mve::FloatImage::ConstPtr &img) {
    mve::FloatImage::Ptr integral_img = mve::FloatImage::create(img->width() + 1, img->height() + 1, 1);

    for (int r = 0; r < img->height(); r++) {
        float sum = 0;
        for (int c = 0; c < img->width(); c++) {
            sum += img->at(c, r, 0);
            integral_img->at(c + 1, r + 1, 0) = integral_img->at(c + 1, r, 0) + sum;
        }
    }
    return integral_img;
}

void MeanFilter(const mve::FloatImage::ConstPtr &img, mve::FloatImage::Ptr &out, int filter_range) {
    out = mve::FloatImage::create(img->width() - 2 * filter_range, img->height() - 2 * filter_range, 1);
    mve::FloatImage::Ptr integral_img = ComputeIntegralImg(img);
    int k_size = 2 * filter_range + 1;
    float normal = 1.f / (k_size * k_size);
#pragma omp parallel for schedule(dynamic, 1)
    for (int r = filter_range + 1; r < img->height() - filter_range + 1; r++) {
#pragma omp parallel for schedule(dynamic, 1)
        for (int c = filter_range + 1; c < img->width() - filter_range + 1; c++) {
            float sum = integral_img->at(c + filter_range, r + filter_range, 0)
                + integral_img->at(c - filter_range - 1, r - filter_range - 1, 0)
                - integral_img->at(c + filter_range, r - filter_range - 1, 0)
                - integral_img->at(c - filter_range - 1, r + filter_range, 0);
            out->at(c - filter_range - 1, r - filter_range - 1, 0) = sum * normal;
        }
    }
}

mve::TriangleMesh::Ptr GenerateMeshSMVS(mve::Scene::Ptr scene,
                                        const std::string &input_name,
                                        const std::string &dm_name,
                                        const std::string &output_name,
                                        bool triangle_mesh) {
    mve::TriangleMesh::Ptr mesh;
    /* Build mesh name */
    std::string ply_path;
    if (util::string::right(output_name, 4) == ".ply") {
        std::cout << ".ply file already exists, skipping mesh generation." << std::endl;
        ply_path = util::fs::join_path(scene->get_path(), output_name);
    } else {
        ply_path = util::fs::join_path(scene->get_path(), output_name + ".ply");
    }
    if (util::fs::file_exists(ply_path.c_str())) { // skip point set reconstruction if ply is found
        std::cout << "The .ply file already exists, skipping mesh generation." << std::endl;
        mesh = mve::geom::load_ply_mesh(ply_path);
    } else {
        std::cout << "Generating Mesh";

        util::WallTimer timer;
        mve::Scene::ViewList recon_views;
        for (auto & i : scene->get_views())
            recon_views.push_back(i);

        std::cout << " for " << recon_views.size() << " views ..." << std::endl;

        smvs::MeshGenerator::Options meshgen_opts;
        meshgen_opts.num_threads = std::thread::hardware_concurrency();
        meshgen_opts.cut_surfaces = true;
        meshgen_opts.simplify = false;
        meshgen_opts.create_triangle_mesh = false;

        smvs::MeshGenerator meshgen(meshgen_opts);
        mesh = meshgen.generate_mesh(recon_views, input_name, dm_name);
        std::cout << "Done. Took: " << timer.get_elapsed_sec() << "s" << std::endl;

        if(triangle_mesh)
            mesh->recalc_normals();

        /* Save mesh */
        mve::geom::SavePLYOptions opts;
        opts.write_vertex_normals = true;
        opts.write_vertex_values = true;
        opts.write_vertex_confidences = true;
        std::cout << "Writing final point set to "<< ply_path << std::endl;
        mve::geom::save_ply_mesh(mesh, ply_path, opts);
    }
    return mesh;
}

mve::TriangleMesh::Ptr GenerateMesh(mve::Scene::Ptr scene,
                                    const std::string &input_name,
                                    const std::string &dm_name,
                                    int scale,
                                    const std::string &output_name) {
    /* Build mesh name */
    std::string ply_path;
    if (util::string::right(output_name, 4) == ".ply") {
        ply_path = util::fs::join_path(scene->get_path(), output_name);
    } else {
        ply_path = util::fs::join_path(scene->get_path(), output_name + ".ply");
    }
    mve::TriangleMesh::Ptr point_set;
    if (util::fs::file_exists(ply_path.c_str())) { // skip point set reconstruction if ply is found
        std::cout << "The .ply file already exists, skipping mesh generation." << std::endl;
        point_set = mve::geom::load_ply_mesh(ply_path);
    } else {
        mve::Scene::ViewList &views(scene->get_views());
        /* Prepare output mesh. */
        point_set = mve::TriangleMesh::create();
        mve::TriangleMesh::VertexList &verts(point_set->get_vertices());
        mve::TriangleMesh::NormalList &vnorm(point_set->get_vertex_normals());
        mve::TriangleMesh::ColorList &vcolor(point_set->get_vertex_colors());
        mve::TriangleMesh::ValueList &vvalues(point_set->get_vertex_values());
        mve::TriangleMesh::ConfidenceList &vconfs(point_set->get_vertex_confidences());


#pragma omp parallel for schedule(dynamic)
        for (std::size_t i = 0; i < views.size(); ++i) {
            mve::View::Ptr view = views[i];
            if (view == nullptr)
                continue;

            mve::CameraInfo const &cam = view->get_camera();
            if (cam.flen == 0.0f)
                continue;

            mve::FloatImage::Ptr dm = view->get_float_image(dm_name);
            if (dm == nullptr) {
#pragma omp critical
                std::cout << "Depth map not found, skipping view ID: " << i << std::endl;
                continue;
            }

            mve::ByteImage::Ptr ci = view->get_byte_image(input_name);
            if (ci == nullptr) {
#pragma omp critical
                std::cout << "Input image not found, skipping view ID: " << i << std::endl;
                std::cout << std::endl;
                continue;
            }

#pragma omp critical
            std::cout << "Processing view \"" << view->get_name()
                      << "\"" << (ci != nullptr ? " (with colors)" : "")
                      << "..." << std::endl;

            /* Triangulate depth map. */
            mve::TriangleMesh::Ptr mesh;

            mve::Image<unsigned int> vertex_ids;
            mesh = mve::geom::depthmap_triangulate(dm, ci, cam, mve::geom::DD_FACTOR_DEFAULT, &vertex_ids);

            mve::TriangleMesh::VertexList const &mverts(mesh->get_vertices());
            mve::TriangleMesh::NormalList const &mnorms(mesh->get_vertex_normals());
            mve::TriangleMesh::ColorList const &mvcol(mesh->get_vertex_colors());
            mve::TriangleMesh::ConfidenceList &mconfs(mesh->get_vertex_confidences());

            mesh->ensure_normals();

            mve::geom::depthmap_mesh_confidences(mesh, 3);

            std::vector<float> mvscale;
            mvscale.resize(mverts.size(), 0.0f);
            mve::MeshInfo mesh_info(mesh);
            for (std::size_t j = 0; j < mesh_info.size(); ++j) {
                mve::MeshInfo::VertexInfo const &vinf = mesh_info[j];
                for (unsigned long long vert : vinf.verts)
                    mvscale[j] += (mverts[j] - mverts[vert]).norm();
                mvscale[j] /= static_cast<float>(vinf.verts.size());
                mvscale[j] *= scale;
            }

#pragma omp critical
            {
                verts.insert(verts.end(), mverts.begin(), mverts.end());
                if (!mvcol.empty())
                    vcolor.insert(vcolor.end(), mvcol.begin(), mvcol.end());
                if (!mnorms.empty())
                    vnorm.insert(vnorm.end(), mnorms.begin(), mnorms.end());
                if (!mvscale.empty())
                    vvalues.insert(vvalues.end(), mvscale.begin(), mvscale.end());
                if (!mconfs.empty())
                    vconfs.insert(vconfs.end(), mconfs.begin(), mconfs.end());
            }
            dm.reset();
            ci.reset();
            view->cache_cleanup();
        }
        /* Write mesh to disc. */
        mve::geom::SavePLYOptions opts;
        opts.write_vertex_normals = true;
        opts.write_vertex_values = true;
        opts.write_vertex_confidences = true;
        std::cout << "Writing final point set to "<< ply_path << std::endl;
        mve::geom::save_ply_mesh(point_set, ply_path, opts);
    }
    return point_set;
}

void reconstructSGMDepthForView(const smvs::SGMStereo::Options &opt,
                                const std::string &outputName,
                                smvs::StereoView::Ptr main_view,
                                std::vector<smvs::StereoView::Ptr> neighbors,
                                mve::Bundle::ConstPtr bundle) {
    util::WallTimer sgm_timer;
    mve::FloatImage::Ptr d1 = smvs::SGMStereo::reconstruct(opt, main_view,
                                                           neighbors[0], bundle);
    if (neighbors.size() > 1) {
        mve::FloatImage::Ptr d2 = smvs::SGMStereo::reconstruct(opt,
                                                               main_view, neighbors[1], bundle);
        for (int p = 0; p < d1->get_pixel_amount(); ++p) {
            if (d2->at(p) == 0.0f)
                continue;
            if (d1->at(p) == 0.0f) {
                d1->at(p) = d2->at(p);
                continue;
            }
            d1->at(p) = (d1->at(p) + d2->at(p)) * 0.5f;
        }
    }

    std::cout << "SGM took: " << sgm_timer.get_elapsed_sec()
              << "sec" << std::endl;

    main_view->write_depth_to_view(d1, outputName);
}

void resizeViews(mve::Scene::ViewList &views,
                 const std::set<int> &list,
                 const std::string &output_name,
                 int scale) {
    if (scale == 0) return;
    std::vector<std::future<void>> resize_tasks;
    ThreadPool thread_pool(std::max<std::size_t>(std::thread::hardware_concurrency(), 1));
    for (auto const &i : list) {
        mve::View::Ptr view = views[i];
        if (view == nullptr
            || !view->has_image(UNDISTORTED_IMAGE_NAME)
            || view->has_image(output_name))
            continue;

        resize_tasks.emplace_back(thread_pool.add_task(
            [view, &output_name, &scale] {
                resizeView(view, output_name, scale);
            }));
    }

    if (!resize_tasks.empty()) {
        std::cout << "Resizing input images for "
                  << resize_tasks.size() << " views... " << std::flush;
        util::WallTimer timer;
        for (auto &&task : resize_tasks)
            task.get();
        std::cout << " done, took " << timer.get_elapsed_sec()
                  << "s." << std::endl;
    }
}

void resizeView(mve::View::Ptr view, const std::string &output_name, int scale) {
    mve::ByteImage::ConstPtr input =
        view->get_byte_image(UNDISTORTED_IMAGE_NAME);
    mve::ByteImage::Ptr scld = input->duplicate();
    for (int i = 0; i < scale; ++i)
        scld = mve::image::rescale_half_size_gaussian<uint8_t>(scld);
    view->set_image(scld, output_name);
    view->save_view();
}

} // namespace Util
