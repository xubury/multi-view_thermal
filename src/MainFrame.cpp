#include "MainFrame.hpp"
#include "GLPanel.hpp"
#include <algorithm>
#include <atomic>
#include "util/system.h"
#include "util/timer.h"
#include "util/file_system.h"
#include "sfm/feature_set.h"
#include "sfm/bundler_common.h"
#include "sfm/bundler_intrinsics.h"
#include "sfm/bundler_tracks.h"
#include "sfm/bundler_init_pair.h"
#include "sfm/bundler_incremental.h"
#include "mve/image_tools.h"
#include "mve/bundle_io.h"
#include "dmrecon/settings.h"
#include "dmrecon/dmrecon.h"
#include "math/octree_tools.h"
#include "mve/depthmap.h"
#include "mve/mesh_info.h"
#include "mve/mesh_io.h"
#include "mve/mesh_io_ply.h"
#include "mve/mesh_tools.h"
#include "Image.hpp"
#include "feature/Harris.hpp"

MainFrame::MainFrame(wxWindow *parent, wxWindowID id, const wxString &title, const wxPoint &pos,
                     const wxSize &size) : wxFrame(parent, id, title, pos, size) {
    wxInitAllImageHandlers();
    auto pImageListCtrl = new wxListCtrl(this, wxID_ANY, wxDefaultPosition,
                                         wxDefaultSize, wxLC_REPORT | wxLC_SINGLE_SEL);
    pImageListCtrl->InsertColumn(0, _("Image"), wxLIST_FORMAT_LEFT, THUMBNAIL_SIZE);
    auto pImageList = new wxImageList(THUMBNAIL_SIZE, THUMBNAIL_SIZE, false);
    pImageListCtrl->SetImageList(pImageList, wxIMAGE_LIST_SMALL);
    m_originalImageList.first = pImageListCtrl;
    m_originalImageList.second = pImageList;

    auto *pBoxSizer = new wxBoxSizer(wxHORIZONTAL);
    int args[] = {WX_GL_CORE_PROFILE,
                  WX_GL_RGBA,
                  WX_GL_DOUBLEBUFFER,
                  WX_GL_DEPTH_SIZE, 16,
                  WX_GL_STENCIL_SIZE, 0,
                  0, 0};
    m_pGLPanel = new GLPanel(this, wxID_ANY, args);
    pBoxSizer->Add(pImageListCtrl, 0, wxEXPAND);
    pBoxSizer->Add(m_pGLPanel, 1, wxEXPAND);
    this->SetSizer(pBoxSizer);

    auto *pMenuBar = new wxMenuBar();

    auto *pFileMenu = new wxMenu();
    pFileMenu->Append(MENU::MENU_SCENE_OPEN, _("Open Scene"));
    pFileMenu->Append(MENU::MENU_SCENE_NEW, _("New Scene"));
    pFileMenu->Bind(wxEVT_MENU, &MainFrame::OnMenuOpenScene, this, MENU::MENU_SCENE_OPEN);
    pFileMenu->Bind(wxEVT_MENU, &MainFrame::OnMenuNewScene, this, MENU::MENU_SCENE_NEW);

    auto *pOperateMenu = new wxMenu();
    pOperateMenu->Append(MENU::MENU_DO_SFM, _("Structure from Motion"));
    pOperateMenu->Bind(wxEVT_MENU, &MainFrame::OnMenuStructureFromMotion, this, MENU::MENU_DO_SFM);
    pOperateMenu->Append(MENU::MENU_DEPTH_RECON, _("Depth Reconstruction"));
    pOperateMenu->Bind(wxEVT_MENU, &MainFrame::OnMenuDepthRecon, this, MENU::MENU_DEPTH_RECON);
    pOperateMenu->Append(MENU::MENU_PSET_RECON, _("Dense Point Cloud Reconstruction"));
    pOperateMenu->Bind(wxEVT_MENU, &MainFrame::OnMenuDensePointRecon, this, MENU::MENU_PSET_RECON);

    pMenuBar->Append(pFileMenu, _("File"));
    pMenuBar->Append(pOperateMenu, _("Operation"));

    this->SetMenuBar(pMenuBar);
    this->CreateStatusBar(1);

    util::system::register_segfault_handler();
}

void MainFrame::OnMenuOpenScene(wxCommandEvent &event) {
    wxDirDialog dlg(this);
    if (dlg.ShowModal() == wxID_OK) {
        util::system::print_build_timestamp("Open MVE Scene");

        std::string aPath = dlg.GetPath().ToStdString();
        aPath = util::fs::sanitize_path(aPath);

        try {
            m_pScene = mve::Scene::create(aPath);
        } catch (const std::exception &e) {
            std::cout << "Error Loading Scene: " << e.what() << std::endl;
            event.Skip();
            return;
        }
        if (m_pScene->get_views().empty()) {
            wxLogMessage(_("Empty scene, please select a scene folder with .mve views."));
            event.Skip();
            return;
        }
        try {
            mve::Bundle::Ptr bundle = mve::load_mve_bundle(util::fs::join_path(m_pScene->get_path(), "synth_0.out"));
            std::vector<Vertex> vertices(bundle->get_features().size());
            mve::Bundle::Features &features = bundle->get_features();
            m_pGLPanel->ClearObjects<Cluster>();
            for (std::size_t i = 0; i < vertices.size(); ++i) {
                vertices[i].Position = glm::vec3(features[i].pos[0], features[i].pos[1], features[i].pos[2]);
                vertices[i].Color = glm::vec3(features[i].color[0], features[i].color[1], features[i].color[2]);
            }
            m_pGLPanel->AddCluster(vertices);

            m_pGLPanel->ClearObjects<Frustum>();
            for (const auto &view : m_pScene->get_views()) {
                if (!view->is_camera_valid())
                    continue;

                glm::mat4 trans;
                view->get_camera().fill_cam_to_world(&trans[0].x);
                trans = glm::transpose(trans);
                m_pGLPanel->AddCameraFrustum(trans);
            }
        } catch (const std::exception &e) {
            std::cout << "Error opening bundle file: " << e.what() << std::endl;
        }

        SetStatusText(aPath);
        DisplaySceneImage(ORIGINAL_IMAGE_NAME, m_originalImageList);
    }
    event.Skip();
}

void MainFrame::OnMenuNewScene(wxCommandEvent &event) {
    wxDirDialog dlg(this);
    if (dlg.ShowModal() == wxID_OK) {
        util::system::print_build_timestamp("New MVE Scene");

        util::WallTimer timer;

        util::fs::Directory dir;

        std::string aPath = dlg.GetPath().ToStdString();
        aPath = util::fs::sanitize_path(aPath);
        std::string scenePath = util::fs::join_path(aPath, "scene");
        std::string viewsPath = util::fs::join_path(scenePath, MVE_SCENE_VIEWS_DIR);
        util::fs::mkdir(scenePath.c_str());
        util::fs::mkdir(viewsPath.c_str());

        try {
            dir.scan(aPath);
        } catch (std::exception &e) {
            std::cerr << "Error scanning input dir: " << e.what() << std::endl;
            event.Skip();
            return;
        }
        std::cout << "Found " << dir.size() << " directory entries" << std::endl;

        std::sort(dir.begin(), dir.end());
        std::atomic_int id_cnt(0);
        std::atomic_int num_imported(0);
#pragma omp parallel for ordered schedule(dynamic, 1)
        for (std::size_t i = 0; i < dir.size(); ++i) {
            if (dir[i].is_dir) {
#pragma omp critical
                std::cout << "Skipping directory " << dir[i].name << std::endl;
                continue;
            }

            std::string fname = dir[i].name;
            std::string afname = dir[i].get_absolute_name();

            std::string exif;
            mve::ImageBase::Ptr image = load_any_image(afname, &exif);
            if (image == nullptr)
                continue;
            int id;
#pragma omp ordered
            id = id_cnt++;
            ++num_imported;

            /* Create view, set headers, add image. */
            mve::View::Ptr view = mve::View::create();
            view->set_id(id);
            view->set_name(remove_file_extension(fname));

            /* Rescale and add original image. */
            int orig_width = image->width();
            int max_pixel = std::numeric_limits<int>::max();
            image = limit_image_size(image, max_pixel);
            if (orig_width == image->width() && has_jpeg_extension(fname))
                view->set_image_ref(afname, ORIGINAL_IMAGE_NAME);
            else
                view->set_image(image, ORIGINAL_IMAGE_NAME);

            add_exif_to_view(view, exif);

            std::string mve_fname = make_image_name(id);
#pragma omp critical
            std::cout << "Importing image: " << fname
                      << ", writing MVE view: " << mve_fname << "..." << std::endl;
            view->save_view_as(util::fs::join_path(viewsPath, mve_fname));
        }
        std::cout << "Imported " << num_imported << " input images, took "
                  << timer.get_elapsed() << " ms." << std::endl;

        m_pScene = mve::Scene::create(scenePath);
        // If there is a bundle file exist in the disk, delete it.
        const std::string prebundle_path = util::fs::join_path(m_pScene->get_path(), "prebundle.sfm");
        if (util::fs::file_exists(prebundle_path.c_str())) {
            std::remove(prebundle_path.c_str());
        }

        SetStatusText(scenePath);
        DisplaySceneImage(ORIGINAL_IMAGE_NAME, m_originalImageList);
    }
    event.Skip();
}

void MainFrame::DisplaySceneImage(const std::string &image_name, const ImageList &image_list) {
    mve::Scene::ViewList &views = m_pScene->get_views();
    if (views.empty()) {
        return;
    }
    wxListCtrl *listCtrl = image_list.first;
    wxImageList *iconList = image_list.second;
    listCtrl->DeleteAllItems();
    for (std::size_t i = 0; i < views.size(); ++i) {
        mve::ByteImage::Ptr image = views[i]->get_byte_image(image_name);
        wxImage icon(image->width(), image->height());
        memcpy(icon.GetData(), image->get_data_pointer(), image->get_byte_size());
        int width = 0;
        int height = 0;
        iconList->GetSize(i, width, height);
        icon.Rescale(width, height);
        if (!iconList->Replace(i, icon)) {
            iconList->Add(icon);
        }
        listCtrl->InsertItem(i, wxString::Format("ID :%d Dir:%s",
                                                 views[i]->get_id(), views[i]->get_directory()), i);
    }
    ////////////////////////////////////////Harris Test////////////////////////////////////////
//    Harris harris(0.04, 3, 1);
//    try {
//        mve::ByteImage::Ptr img = views[0]->get_byte_image("original");
//        harris.SetImage(img);
//        harris.Process();
//        Harris::KeyPoints pts = harris.GetMaximaPoints(0.01, 5);
//        wxMemoryDC dc;
//        wxImage wximg(img->width(), img->height());
//        memcpy(wximg.GetData(), img->get_data_pointer(), img->get_byte_size());
//        wxBitmap bitmap = wximg;
//        dc.SelectObject(bitmap);
//        float max = pts[0].corner_response;
//        for (const auto &pt : pts) {
//            if (pt.corner_response > 0.01 * max)
//                dc.DrawCircle(pt.x, pt.y, 2);
//        }
//        dc.SelectObject(wxNullBitmap);
//        bitmap.SaveFile("test.jpg", wxBITMAP_TYPE_JPEG);
//    } catch (const std::exception &e) {
//        std::cout << e.what() << std::endl;
//    }
    ////////////////////////////////////////Harris Test////////////////////////////////////////
    for (int i = 0; i < listCtrl->GetColumnCount(); ++i)
        listCtrl->SetColumnWidth(i, -1);
}

void MainFrame::OnMenuStructureFromMotion(wxCommandEvent &event) {
    if (m_pScene == nullptr) {
        event.Skip();
        return;
    }
    // prebundle.sfm is for holding view and view matching info
    const std::string prebundle_path = util::fs::join_path(m_pScene->get_path(), "prebundle.sfm");
    sfm::bundler::ViewportList viewPorts;
    sfm::bundler::PairwiseMatching pairwise_matching;
    if (!util::fs::file_exists(prebundle_path.c_str())) {
        std::cout << "Start feature matching." << std::endl;
        util::system::rand_seed(RAND_SEED_MATCHING);
        if (!features_and_matching(m_pScene, &viewPorts, &pairwise_matching)) {
            event.Skip();
            return; // no feature match
        }

        std::cout << "Saving pre-bundle to file..." << std::endl;
        sfm::bundler::save_prebundle_to_file(viewPorts, pairwise_matching, prebundle_path);
    } else {
        std::cout << "Loading pairwise matching from file..." << std::endl;
        sfm::bundler::load_prebundle_from_file(prebundle_path, &viewPorts, &pairwise_matching);
    }

    /* Drop descriptors and embeddings to save memory. */
    m_pScene->cache_cleanup();
    for (auto &viewPort : viewPorts)
        viewPort.features.clear_descriptors();

    /* Check if there are some matching images. */
    if (pairwise_matching.empty()) {
        std::cerr << "No matching image pairs." << std::endl;
        event.Skip();
        return;
    }

    /* Load camera intrinsics. */
    {
        sfm::bundler::Intrinsics::Options intrinsics_opts;
        if (m_pScene->get_views().back()->has_blob("exif")) {
            //get camera intrinsics from exif
            std::cout << "Initializing camera intrinsics from exif..." << std::endl;
            intrinsics_opts.intrinsics_source = sfm::bundler::Intrinsics::FROM_EXIF;
        } else {
            // get camera intrinsics from views
            // (if meta.ini has no intrinsics when loading the scene, then it get default value)
            std::cout << "Initializing camera intrinsics from views..." << std::endl;
            intrinsics_opts.intrinsics_source = sfm::bundler::Intrinsics::FROM_VIEWS;
        }
        sfm::bundler::Intrinsics intrinsics(intrinsics_opts);
        intrinsics.compute(m_pScene, &viewPorts);
    }

    /** Incremental SfM*/
    util::WallTimer timer;
    util::system::rand_seed(RAND_SEED_SFM);

    sfm::bundler::TrackList tracks;
    {
        sfm::bundler::Tracks::Options tracks_opts;
        tracks_opts.verbose_output = true;

        sfm::bundler::Tracks bundler_tracks(tracks_opts);
        std::cout << "Computing feature tracks..." << std::endl;
        bundler_tracks.compute(pairwise_matching, &viewPorts, &tracks);
        std::cout << "Create a total of " << tracks.size() << " tracks." << std::endl;
    }

    /** Remove color data and pairwise matching to save memory*/
    for (auto &viewPort : viewPorts) {
        viewPort.features.colors.clear();
        viewPort.features.colors.shrink_to_fit();
    }
    pairwise_matching.clear();
    pairwise_matching.shrink_to_fit();

    /* Search for a good initial pair*/
    //TODO: add a option for user to specify init pair manually
    sfm::bundler::InitialPair::Result init_pair_result;
    sfm::bundler::InitialPair::Options init_pair_opts;
    //init_pair_opts.homography_opts.max_iterations = 1000;
    //init_pair_opts.homography_opts.threshold = 0.005f;
    init_pair_opts.homography_opts.verbose_output = false;
    init_pair_opts.max_homography_inliers = 0.8f;
    init_pair_opts.verbose_output = true;

    sfm::bundler::InitialPair init_pair(init_pair_opts);
    init_pair.initialize(viewPorts, tracks);
    init_pair.compute_pair(&init_pair_result);

    if (init_pair_result.view_1_id < 0 || init_pair_result.view_2_id < 0
        || init_pair_result.view_1_id >= static_cast<int>(viewPorts.size())
        || init_pair_result.view_2_id >= static_cast<int>(viewPorts.size())) {
        std::cerr << "Error finding initial pair, exiting!" << std::endl;
        std::cerr << "Try manually specifying an initial pair." << std::endl;
        event.Skip();
        return;
    }
    std::cout << "Using views " << init_pair_result.view_1_id
              << " and " << init_pair_result.view_2_id
              << " as initial pair." << std::endl;

    /* Incrementally compute full bundle. */
    sfm::bundler::Incremental::Options incremental_opts;
    //incremental_opts.pose_p3p_opts.max_iterations = 1000;
    //incremental_opts.pose_p3p_opts.threshold = 0.005f;
    incremental_opts.pose_p3p_opts.verbose_output = false;
    incremental_opts.track_error_threshold_factor = 10.0f;
    incremental_opts.new_track_error_threshold = 0.01f;
    incremental_opts.min_triangulation_angle = MATH_DEG2RAD(1.0);
    incremental_opts.ba_fixed_intrinsics = false;
    //incremental_opts.ba_shared_intrinsics = conf.shared_intrinsics;
    incremental_opts.verbose_output = true;
    incremental_opts.verbose_ba = false;

    viewPorts[init_pair_result.view_1_id].pose = init_pair_result.view_1_pose;
    viewPorts[init_pair_result.view_2_id].pose = init_pair_result.view_2_pose;

    sfm::bundler::Incremental incremental(incremental_opts);
    incremental.initialize(&viewPorts, &tracks);
    incremental.triangulate_new_tracks(2);
    incremental.invalidate_large_error_tracks();

    std::cout << "Running full bundle adjustment..." << std::endl;
    incremental.bundle_adjustment_full();

    int num_cameras_reconstructed = 2;
    int full_ba_num_skipped = 0;
    while (true) {
        std::vector<int> next_views;
        incremental.find_next_views(&next_views);

        int next_view_id = -1;
        for (int next_view : next_views) {
            std::cout << "Add next view ID " << next_view
                      << "(" << num_cameras_reconstructed + 1 << " of " << viewPorts.size() << ")...\n";
            if (incremental.reconstruct_next_view(next_view)) {
                next_view_id = next_view;
                break;
            }
        }
        std::flush(std::cout);
        if (next_view_id < 0) {
            if (full_ba_num_skipped == 0) {
                std::cout << "No valid next view\n";
                std::cout << "SfM reconstruction finished" << std::endl;
                break;
            } else {
                incremental.triangulate_new_tracks(3);
                std::cout << "Running full bundle adjustment..." << std::endl;
                incremental.bundle_adjustment_full();
                incremental.invalidate_large_error_tracks();
                full_ba_num_skipped = 0;
                continue;
            }
        }
        /* Run single-camera bundle adjustment. */
        std::cout << "Running single camera bundle adjustment..." << std::endl;
        incremental.bundle_adjustment_single_cam(next_view_id);
        num_cameras_reconstructed += 1;

        /* Run full bundle adjustment only after a couple of views. */
        const int full_ba_skip_views = std::min(100, num_cameras_reconstructed / 10);
        if (full_ba_num_skipped < full_ba_skip_views) {
            std::cout << "Skipping full bundle adjustment (skipping "
                      << full_ba_skip_views << " views)." << std::endl;
            full_ba_num_skipped += 1;
        } else {
            incremental.triangulate_new_tracks(3);
            incremental.try_restore_tracks_for_views();
            std::cout << "Running full bundle adjustment..." << std::endl;
            incremental.bundle_adjustment_full();
            incremental.invalidate_large_error_tracks();
            full_ba_num_skipped = 0;
        }
    }

    std::cout << "SfM reconstruction took " << timer.get_elapsed()
              << " ms." << std::endl;

    std::cout << "Creating bundle data structure..." << std::endl;
    mve::Bundle::Ptr bundle = incremental.create_bundle();
    mve::save_mve_bundle(bundle, util::fs::join_path(m_pScene->get_path(), "synth_0.out"));
    std::vector<Vertex> vertices(bundle->get_features().size());
    mve::Bundle::Features &features = bundle->get_features();
    m_pGLPanel->ClearObjects<Cluster>();
    for (std::size_t i = 0; i < vertices.size(); ++i) {
        vertices[i].Position = glm::vec3(features[i].pos[0], features[i].pos[1], features[i].pos[2]);
        vertices[i].Color = glm::vec3(features[i].color[0], features[i].color[1], features[i].color[2]);
    }
    m_pGLPanel->AddCluster(vertices);

    /* Apply bundle cameras to views. */
    mve::Bundle::Cameras const &bundle_cams = bundle->get_cameras();
    mve::Scene::ViewList const &views = m_pScene->get_views();
    if (bundle_cams.size() != views.size()) {
        std::cerr << "Error: Invalid number of cameras!" << std::endl;
        event.Skip();
        return;
    }

#pragma omp parallel for schedule(dynamic, 1)
    for (std::size_t i = 0; i < bundle_cams.size(); ++i) {
        mve::View::Ptr view = views[i];
        mve::CameraInfo const &cam = bundle_cams[i];
        if (view == nullptr)
            continue;
        if (view->get_camera().flen == 0.0f && cam.flen == 0.0f)
            continue;

        view->set_camera(cam);

        /* Undistort image. */
        mve::ByteImage::Ptr original
            = view->get_byte_image(ORIGINAL_IMAGE_NAME);
        if (original == nullptr)
            continue;
        mve::ByteImage::Ptr undist
            = mve::image::image_undistort_k2k4<uint8_t>
                (original, cam.flen, cam.dist[0], cam.dist[1]);
        view->set_image(undist, UNDISTORTED_IMAGE_NAME);

#pragma omp critical
        std::cout << "Saving view " << view->get_directory() << std::endl;
        view->save_view();
        view->cache_cleanup();
    }
    m_pGLPanel->ClearObjects<Frustum>();
    for (const auto &view : views) {
        glm::mat4 trans;
        view->get_camera().fill_cam_to_world(&trans[0].x);
        trans = glm::transpose(trans);
        m_pGLPanel->AddCameraFrustum(trans);
    }
}

void MainFrame::OnMenuDepthRecon(wxCommandEvent &event) {
    if (m_pScene == nullptr) {
        event.Skip();
        return;
    }
    util::WallTimer timer;
    mvs::Settings settings;
    settings.scale = get_scale_from_max_pixel(m_pScene, settings);
    mve::Scene::ViewList &views(m_pScene->get_views());
    if (views.empty()) {
        event.Skip();
        return;
    } else {
#pragma omp parallel for schedule(dynamic, 1)
        for (std::size_t id = 0; id < views.size(); ++id) {
            if (views[id] == nullptr || !views[id]->is_camera_valid())
                continue;

            /* Setup MVS. */
            settings.refViewNr = id;

            std::string embedding_name = "depth-L"
                + util::string::get(settings.scale);
            if (views[id]->has_image(embedding_name))
                continue;

            try {
                mvs::DMRecon recon(m_pScene, settings);
                recon.start();
                views[id]->save_view();
            }
            catch (std::exception &err) {
                std::cerr << err.what() << std::endl;
            }
        }
    }
    std::cout << "Reconstruction took "
              << timer.get_elapsed() << "ms." << std::endl;
    std::cout << "Saving views back to disc..." << std::endl;
    m_pScene->save_views();
    event.Skip();
}

void MainFrame::OnMenuDensePointRecon(wxCommandEvent &event) {
    if (m_pScene == nullptr) {
        event.Skip();
        return;
    }
    std::string ply_path = util::fs::join_path(m_pScene->get_path(), "point-set.ply");
    mve::TriangleMesh::Ptr point_set;
    if (util::fs::file_exists(ply_path.c_str())) // skip point set reconstruction if ply is found
        point_set = mve::geom::load_ply_mesh(ply_path);
    else {
        /* Prepare output mesh. */
        point_set = mve::TriangleMesh::create();
        mve::TriangleMesh::VertexList &verts(point_set->get_vertices());
        mve::TriangleMesh::NormalList &vnorm(point_set->get_vertex_normals());
        mve::TriangleMesh::ColorList &vcolor(point_set->get_vertex_colors());
        mve::TriangleMesh::ValueList &vvalues(point_set->get_vertex_values());
        mve::TriangleMesh::ConfidenceList &vconfs(point_set->get_vertex_confidences());

        /* Iterate over views and get points. */
        mve::Scene::ViewList &views(m_pScene->get_views());

        mvs::Settings settings;
        int scale = get_scale_from_max_pixel(m_pScene, settings);

#pragma omp parallel for schedule(dynamic)
        for (std::size_t i = 0; i < views.size(); ++i) {
            mve::View::Ptr view = views[i];
            if (view == nullptr)
                continue;

            mve::CameraInfo const &cam = view->get_camera();
            if (cam.flen == 0.0f)
                continue;

            mve::FloatImage::Ptr dm = view->get_float_image("depth-L" + std::to_string(scale));
            if (dm == nullptr)
                continue;

            mve::ByteImage::Ptr ci;
            if (scale != 0)
                ci = view->get_byte_image("undist-L" + std::to_string(scale));
            else
                ci = view->get_byte_image("undistorted");

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
            mve::geom::depthmap_mesh_confidences(mesh, 4);
            std::vector<float> mvscale;
            mvscale.resize(mverts.size(), 0.0f);
            mve::MeshInfo mesh_info(mesh);
            for (std::size_t j = 0; j < mesh_info.size(); ++j) {
                mve::MeshInfo::VertexInfo const &vinf = mesh_info[j];
                for (std::size_t k = 0; k < vinf.verts.size(); ++k)
                    mvscale[j] += (mverts[j] - mverts[vinf.verts[k]]).norm();
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
        std::cout << "Writing final point set ("
                  << verts.size() << " points)..." << std::endl;
        mve::geom::save_mesh(point_set, util::fs::join_path(m_pScene->get_path(), "point-set.ply"));
    }

    // display cluster
    mve::TriangleMesh::VertexList &v_pos(point_set->get_vertices());
    mve::TriangleMesh::ColorList &v_color(point_set->get_vertex_colors());
    std::vector<Vertex> vertices(v_pos.size());
    glm::mat4 transform(1.0f);
    if (!m_pGLPanel->GetTargetList().empty()) {
        transform = m_pGLPanel->GetTargetList().back()->GetTransform();
    }
    m_pGLPanel->ClearObjects<Cluster>();
    for (std::size_t i = 0; i < vertices.size(); ++i) {
        vertices[i].Position = glm::vec3(v_pos[i][0], v_pos[i][1], v_pos[i][2]);
        vertices[i].Color = glm::vec3(v_color[i][0], v_color[i][1], v_color[i][2]);
    }
    m_pGLPanel->AddCluster(vertices, transform);
    Refresh();

    event.Skip();
}
