#include "MainFrame.hpp"
#include "GLPanel.hpp"
#include <algorithm>
#include <atomic>
#include <dmrecon/dmrecon.h>
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
#include "math/octree_tools.h"
#include "mve/mesh_info.h"
#include "mve/mesh_io.h"
#include "mve/mesh_io_ply.h"
#include "mve/mesh_tools.h"
#include "fssr/sample_io.h"
#include "fssr/iso_octree.h"
#include "fssr/iso_surface.h"
#include "fssr/mesh_clean.h"
#include "Image.hpp"
#include "Util.hpp"

#include "thread_pool.h"
#include "stereo_view.h"
#include "depth_optimizer.h"
#include "view_selection.h"

MainFrame::MainFrame(wxWindow *parent, wxWindowID id, const wxString &title, const wxPoint &pos,
                     const wxSize &size) : wxFrame(parent, id, title, pos, size), m_scale(0) {
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
    pOperateMenu->Append(MENU::MENU_DISPLAY_FRUSTUM, _("Display Frustum"), wxEmptyString, true);
    pOperateMenu->Bind(wxEVT_MENU, &MainFrame::OnMenuDisplayFrustum, this, MENU::MENU_DISPLAY_FRUSTUM);
    pOperateMenu->Append(MENU::MENU_DEPTH_RECON_MVS, _("Dense reconstruction(MVS)"));
    pOperateMenu->Bind(wxEVT_MENU, &MainFrame::OnMenuDepthReconMVS, this, MENU::MENU_DEPTH_RECON_MVS);
    pOperateMenu->Append(MENU::MENU_DEPTH_RECON_MVS_THERMAL, _("Thermal Dense reconstruction(MVS)"));
    pOperateMenu->Bind(wxEVT_MENU, &MainFrame::OnMenuDepthReconMVS, this, MENU::MENU_DEPTH_RECON_MVS_THERMAL);
    pOperateMenu->Append(MENU::MENU_DEPTH_RECON_SHADING, _("Dense reconstruction(SMVS)"));
    pOperateMenu->Bind(wxEVT_MENU, &MainFrame::OnMenuDepthReconShading, this, MENU::MENU_DEPTH_RECON_SHADING);
    pOperateMenu->Append(MENU::MENU_DEPTH_RECON_SHADING_THERMAL, _("Thermal dense reconstruction(SMVS)"));
    pOperateMenu->Bind(wxEVT_MENU, &MainFrame::OnMenuDepthReconShading, this, MENU::MENU_DEPTH_RECON_SHADING_THERMAL);
    pOperateMenu->Append(MENU::MENU_MESH_RECON_MVS, _("Mesh reconstruction(MVS)"));
    pOperateMenu->Bind(wxEVT_MENU, &MainFrame::OnMenuMeshReconstruction, this, MENU::MENU_MESH_RECON_MVS);
    pOperateMenu->Append(MENU::MENU_MESH_RECON_SHADING, _("Mesh reconstruction(SMVS)"));
    pOperateMenu->Bind(wxEVT_MENU, &MainFrame::OnMenuMeshReconstruction, this, MENU::MENU_MESH_RECON_SHADING);
    pOperateMenu->Append(MENU::MENU_FSS_RECON, _("FSSR reconstruction"));
    pOperateMenu->Bind(wxEVT_MENU, &MainFrame::OnMenuFSSR, this, MENU::MENU_FSS_RECON);

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
            wxLogMessage(_("Error loading scene."));
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

            if (m_pCluster != nullptr)
                m_pGLPanel->ClearObject(m_pCluster);

            for (std::size_t i = 0; i < vertices.size(); ++i) {
                vertices[i].Position = glm::vec3(features[i].pos[0], features[i].pos[1], features[i].pos[2]);
                vertices[i].Color = glm::vec3(features[i].color[0], features[i].color[1], features[i].color[2]);
            }
            m_pCluster = m_pGLPanel->AddCluster(vertices);
        } catch (const std::exception &e) {
            std::cout << "Error opening bundle file: " << e.what() << std::endl;
        }

        m_scale = get_scale_from_max_pixel(m_pScene);
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
        m_scale = get_scale_from_max_pixel(m_pScene);
        m_pGLPanel->ClearObjects<Frustum>();
        m_pGLPanel->ClearObject(m_pCluster);
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
    util::WallTimer total_timer;
    // prebundle.sfm is for holding view and view matching info
    const std::string prebundle_path = util::fs::join_path(m_pScene->get_path(), "prebundle.sfm");
    sfm::bundler::ViewportList viewPorts;
    sfm::bundler::PairwiseMatching pairwise_matching;
    if (!util::fs::file_exists(prebundle_path.c_str())) {
        std::cout << "Start feature matching." << std::endl;
        util::system::rand_seed(RAND_SEED_MATCHING);
        if (!features_and_matching(m_pScene, &viewPorts, &pairwise_matching, sfm::FeatureSet::FEATURE_SIFT)) {
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
    }
    pairwise_matching.clear();

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

    std::cout << "SfM reconstruction took " << total_timer.get_elapsed()
              << " ms." << std::endl;

    std::cout << "Creating bundle data structure..." << std::endl;
    mve::Bundle::Ptr bundle = incremental.create_bundle();
    mve::save_mve_bundle(bundle, util::fs::join_path(m_pScene->get_path(), "synth_0.out"));
    std::vector<Vertex> vertices(bundle->get_features().size());
    mve::Bundle::Features &features = bundle->get_features();
    if (m_pCluster != nullptr)
        m_pGLPanel->ClearObject(m_pCluster);
    for (std::size_t i = 0; i < vertices.size(); ++i) {
        vertices[i].Position = glm::vec3(features[i].pos[0], features[i].pos[1], features[i].pos[2]);
        vertices[i].Color = glm::vec3(features[i].color[0], features[i].color[1], features[i].color[2]);
    }
    m_pCluster = m_pGLPanel->AddCluster(vertices);

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
}

void MainFrame::OnMenuDepthReconShading(wxCommandEvent &event) {
    if (m_pScene == nullptr || m_pScene->get_views().empty()) {
        event.Skip();
        return;
    }
    std::string input_name;
    std::string dm_name;
	std::string sgmName;
	std::string pointset_name;
    smvs::SGMStereo::Options opt;
    int scale = 0;
    bool noOptimize = false;
	if (event.GetId() == MENU_DEPTH_RECON_SHADING) {
        scale = m_scale;
		if (m_scale != 0)
			input_name = "undist-L" + util::string::get(m_scale);
		else
			input_name = UNDISTORTED_IMAGE_NAME;
		dm_name = "smvs-visual-B" + util::string::get(m_scale);
		sgmName = "smvs-visual-SGM";
		pointset_name = "smvs-visual-point-set";
	} else if (event.GetId() == MENU_DEPTH_RECON_SHADING_THERMAL) {
		input_name = "thermal";
		dm_name = "smvs-thermal-B" + util::string::get(m_scale);
		sgmName = "smvs-thermal-SGM";
		pointset_name = "smvs-thermal-point-set";
        noOptimize = true;
	}
    reconstructSMVS(opt, scale, noOptimize, input_name, dm_name, sgmName);
    m_point_set = Util::GenerateMeshSMVS(m_pScene, input_name, dm_name, pointset_name, false);
    // display cluster
    mve::TriangleMesh::VertexList &v_pos(m_point_set->get_vertices());
    mve::TriangleMesh::ColorList &v_color(m_point_set->get_vertex_colors());
    std::vector<Vertex> vertices;
    glm::mat4 transform(1.0f);
    // inherit cluster's transform
    if (m_pCluster != nullptr) {
        transform = m_pCluster->GetTransform();
        m_pGLPanel->ClearObject(m_pCluster);
    }
    for (std::size_t i = 0; i < v_pos.size(); ++i) {
        // not sampling black area
        if (v_color[i][0] < 1e-1 && v_color[i][1] < 1e-1 && v_color[i][2] < 1e-1) {
            continue;
        }
        vertices.emplace_back();
        vertices.back().Position = glm::vec3(v_pos[i][0], v_pos[i][1], v_pos[i][2]);
        vertices.back().Color = glm::vec3(v_color[i][0], v_color[i][1], v_color[i][2]);
    }
    m_pCluster = m_pGLPanel->AddCluster(vertices, transform);
    Refresh();
    event.Skip();
}

void MainFrame::reconstructSMVS(const smvs::SGMStereo::Options &opt,
                                int scale,bool noOptimize,
                                const std::string &input_name,
								const std::string &dm_name,
								const std::string &sgmName
                                ) {
	util::WallTimer total_timer;
    mve::Scene::ViewList &views(m_pScene->get_views());

    /* Add views to reconstruction list */
    std::vector<int> reconstruction_list;
    for (std::size_t i = 0; i < views.size(); ++i) {
        if (views[i] == nullptr) {
            std::cout << "View ID " << i << " invalid, skipping view."
                      << std::endl;
            continue;
        }
        if (!views[i]->has_image(UNDISTORTED_IMAGE_NAME)) {
            std::cout << "View ID " << i << " missing image embedding, "
                      << "skipping view." << std::endl;
            continue;
        }
        if (views[i]->has_image(dm_name)) {
            std::cout << "View ID " << i << " already reconstructed, "
                      << "skipping view." << std::endl;
            continue;
        }
        if (!views[i]->has_image(input_name) && scale == 0) {
            std::cout << "View ID " << i << " missing input image, "
                      << "skipping view." << std::endl;
            continue;

        }
        reconstruction_list.push_back(i);
    }
    /* Create reconstruction threads */
    ThreadPool thread_pool(std::max<std::size_t>(std::thread::hardware_concurrency(), 1));
    /* View selection */
    smvs::ViewSelection::Options view_select_opts;
    view_select_opts.num_neighbors = 6;
    view_select_opts.embedding = UNDISTORTED_IMAGE_NAME;
    smvs::ViewSelection view_selection(view_select_opts, views, m_pScene->get_bundle());
    std::vector<mve::Scene::ViewList> view_neighbors(reconstruction_list.size());
    std::vector<std::future<void>> selection_tasks;
    for (std::size_t v = 0; v < reconstruction_list.size(); ++v) {
        int const i = reconstruction_list[v];
        selection_tasks.emplace_back(thread_pool.add_task(
            [i, v, &view_selection, &view_neighbors] {
              view_neighbors[v] = view_selection.get_neighbors_for_view(i);
            }));
    }
    if (!selection_tasks.empty()) {
        std::cout << "Running view selection for "
                  << selection_tasks.size() << " views... " << std::flush;
        util::WallTimer timer;
        for (auto &&task : selection_tasks)
            task.get();
        std::cout << " done, took " << timer.get_elapsed_sec()
                  << "s." << std::endl;
    }

    std::vector<int> skipped;
    std::vector<int> final_reconstruction_list;
    std::vector<mve::Scene::ViewList> final_view_neighbors;
    for (std::size_t v = 0; v < reconstruction_list.size(); ++v)
        if (view_neighbors[v].size() < view_select_opts.num_neighbors)
            skipped.push_back(reconstruction_list[v]);
        else {
            final_reconstruction_list.push_back(reconstruction_list[v]);
            final_view_neighbors.push_back(view_neighbors[v]);
        }
    if (!skipped.empty()) {
        std::cout << "Skipping " << skipped.size() << " views with "
                  << "insufficient number of neighbors." << std::endl;
        std::cout << "Skipped IDs: ";
        for (std::size_t s = 0; s < skipped.size(); ++s) {
            std::cout << skipped[s] << " ";
            if (s > 0 && s % 12 == 0)
                std::cout << std::endl << "     ";
        }
        std::cout << std::endl;
    }
    reconstruction_list = final_reconstruction_list;
    view_neighbors = final_view_neighbors;

    /* Create input embedding and resize */
    std::set<int> check_embedding_list;
    for (std::size_t v = 0; v < reconstruction_list.size(); ++v) {
        check_embedding_list.insert(reconstruction_list[v]);
        for (auto &neighbor : view_neighbors[v])
            check_embedding_list.insert(neighbor->get_id());
    }

    Util::resizeViews(views, check_embedding_list, input_name, scale);

    std::vector<std::future<void>> results;
    std::mutex counter_mutex;
    std::size_t started = 0;
    std::size_t finished = 0;
    util::WallTimer timer;
    bool useShading = false;

    for (std::size_t v = 0; v < reconstruction_list.size(); ++v) {
        int const i = reconstruction_list[v];
        results.emplace_back(thread_pool.add_task(
            [v, i, &views, &counter_mutex, &opt, &input_name, &dm_name, &sgmName,
                &started, &finished, &reconstruction_list, &view_neighbors, &view_select_opts, &useShading,
                &noOptimize, this] {
              smvs::StereoView::Ptr main_view = smvs::StereoView::create(views[i], input_name, useShading);
              mve::Scene::ViewList neighbors = view_neighbors[v];

              std::vector<smvs::StereoView::Ptr> stereo_views;

              std::unique_lock<std::mutex> lock(counter_mutex);
              std::cout << "\rStarting "
                        << ++started << "/" << reconstruction_list.size()
                        << " ID: " << i
                        << " Neighbors: ";
              for (std::size_t n = 0; n < view_select_opts.num_neighbors
                  && n < neighbors.size(); ++n)
                  std::cout << neighbors[n]->get_id() << " ";
              std::cout << std::endl;
              lock.unlock();

              for (std::size_t n = 0; n < view_select_opts.num_neighbors
                  && n < neighbors.size(); ++n) {
                  smvs::StereoView::Ptr sv = smvs::StereoView::create(
                      neighbors[n], input_name);
                  stereo_views.push_back(sv);
              }

              int sgm_width = views[i]->get_image_proxy(input_name)->width;
              int sgm_height = views[i]->get_image_proxy(input_name)->height;
              sgm_width = (sgm_width + 1) / 2;
              sgm_height = (sgm_height + 1) / 2;
              if (!views[i]->has_image(sgmName)
                  || views[i]->get_image_proxy(sgmName)->width !=
                      sgm_width
                  || views[i]->get_image_proxy(sgmName)->height !=
                      sgm_height)
                  Util::reconstructSGMDepthForView(opt, sgmName, main_view, stereo_views, m_pScene->get_bundle());

              if (noOptimize) return;

              smvs::DepthOptimizer::Options do_opts;
              do_opts.regularization = 0.01;
              do_opts.num_iterations = 5;
              do_opts.min_scale = 2;
              do_opts.output_name = dm_name;
			  do_opts.sgm_name = sgmName;
              do_opts.use_sgm = true;
              do_opts.use_shading = useShading;

              smvs::DepthOptimizer optimizer(main_view, stereo_views,
                                                m_pScene->get_bundle(), do_opts);
              optimizer.optimize();

              std::unique_lock<std::mutex> lock2(counter_mutex);
              std::cout << "\rFinished "
                        << ++finished << "/" << reconstruction_list.size()
                        << " ID: " << i
                        << std::endl;
              lock2.unlock();
            }));
    }
    /* Wait for reconstruction to finish */
    for (auto &&result: results)
        result.get();
    std::cout << "Reconstruction took "
              << total_timer.get_elapsed() << "ms." << std::endl;
    std::cout << "Saving views back to disc..." << std::endl;
    m_pScene->save_views();
}

void MainFrame::OnMenuMeshReconstruction(wxCommandEvent &event) {
    if (m_pScene == nullptr || m_pScene->get_views().empty()) {
        event.Skip();
        return;
    }
    std::string input_name;
    std::string output_name;
    std::string dm_name;
    if (event.GetId() == MENU::MENU_MESH_RECON_MVS) {
        input_name = "merged-mvs";
        output_name = "thermal-mvs";
        dm_name = "smvs-visual-B" + util::string::get(m_scale);
        m_point_set = Util::GenerateMesh(m_pScene, input_name, dm_name, m_scale, output_name);
    } else if (event.GetId() == MENU::MENU_MESH_RECON_SHADING) {
        input_name = "merged-smvs";
        output_name = "thermal-shading";
        dm_name = "smvs-visual-B" + util::string::get(m_scale);
        m_point_set = Util::GenerateMeshSMVS(m_pScene, input_name, dm_name, output_name, false);
    }

    // display cluster
    mve::TriangleMesh::VertexList &v_pos(m_point_set->get_vertices());
    mve::TriangleMesh::ColorList &v_color(m_point_set->get_vertex_colors());
    std::vector<Vertex> vertices;
    glm::mat4 transform(1.0f);
    // inherit cluster's transform
    if (m_pCluster != nullptr) {
        transform = m_pCluster->GetTransform();
        m_pGLPanel->ClearObject(m_pCluster);
    }
    for (std::size_t i = 0; i < v_pos.size(); ++i) {
        // not sampling black area
        if (v_color[i][0] < 1e-1 && v_color[i][1] < 1e-1 && v_color[i][2] < 1e-1) {
            continue;
        }
        vertices.emplace_back();
        vertices.back().Position = glm::vec3(v_pos[i][0], v_pos[i][1], v_pos[i][2]);
        vertices.back().Color = glm::vec3(v_color[i][0], v_color[i][1], v_color[i][2]);
    }
    m_pCluster = m_pGLPanel->AddCluster(vertices, transform);
    Refresh();
    event.Skip();
}

void MainFrame::OnMenuFSSR(wxCommandEvent &event) {
    std::string mesh_name = util::fs::join_path(m_pScene->get_path(), "surface.ply");
    mve::TriangleMesh::Ptr mesh;
    if (!util::fs::file_exists(mesh_name.c_str())) {
        util::WallTimer total_timer;
        if (m_point_set == nullptr) {
            std::cout << "No point set loaded" << std::endl;
            event.Skip();
            return;
        }
        fssr::IsoOctree octree;
        mve::TriangleMesh::VertexList const& verts = m_point_set->get_vertices();
        mve::TriangleMesh::NormalList const& vnormals = m_point_set->get_vertex_normals();
        if (!m_point_set->has_vertex_normals())
            throw std::invalid_argument("Vertex normals missing!");

        mve::TriangleMesh::ValueList const& vvalues = m_point_set->get_vertex_values();
        if (!m_point_set->has_vertex_values())
            throw std::invalid_argument("Vertex scale missing!");

        mve::TriangleMesh::ConfidenceList& vconfs = m_point_set->get_vertex_confidences();
        if (!m_point_set->has_vertex_confidences())
        {
            std::cout << "INFO: No confidences given, setting to 1." << std::endl;
            vconfs.resize(verts.size(), 1.0f);
        }
        mve::TriangleMesh::ColorList& vcolors = m_point_set->get_vertex_colors();
        if (!m_point_set->has_vertex_colors())
            vcolors.resize(verts.size(), math::Vec4f(-1.0f));

        /* Add samples to the list. */
        for (std::size_t i = 0; i < verts.size(); i += 1)
        {
            fssr::Sample sample;
            // not sampling black area
            if (vcolors[i][0] < 1e-4 && vcolors[i][1] < 1e-4 && vcolors[i][2] < 1e-4) {
                continue;
            }
            sample.pos = verts[i];
            sample.normal = vnormals[i];
            sample.scale = vvalues[i];
            sample.confidence = vconfs[i];
            sample.color = math::Vec3f(*vcolors[i]);

            octree.insert_sample(sample);
        }

        /* Exit if no samples have been inserted. */
        if (octree.get_num_samples() == 0)
        {
            std::cerr << "Octree does not contain any samples, exiting."
                      << std::endl;
            std::exit(EXIT_FAILURE);
        }

        /* Compute voxels. */
        octree.limit_octree_level();
        octree.print_stats(std::cout);
        octree.compute_voxels();
        octree.clear_samples();

        /* Extract isosurface. */
        {
            std::cout << "Extracting isosurface..." << std::endl;
            util::WallTimer timer;
            fssr::IsoSurface iso_surface(&octree, fssr::INTERPOLATION_CUBIC);
            mesh = iso_surface.extract_mesh();
            std::cout << "  Done. Surface extraction took "
                      << timer.get_elapsed() << "ms." << std::endl;
        }
        octree.clear();

        /* Check if anything has been extracted. */
        if (mesh->get_vertices().empty())
        {
            std::cerr << "Isosurface does not contain any vertices, exiting."
                      << std::endl;
            std::exit(EXIT_FAILURE);
        }

        /* Surfaces between voxels with zero confidence are ghosts. */
        {
            std::cout << "Deleting zero confidence vertices..." << std::flush;
            util::WallTimer timer;
            std::size_t num_vertices = mesh->get_vertices().size();
            mve::TriangleMesh::DeleteList delete_verts(num_vertices, false);
            for (std::size_t i = 0; i < num_vertices; ++i)
                if (mesh->get_vertex_confidences()[i] == 0.0f)
                    delete_verts[i] = true;
            mesh->delete_vertices_fix_faces(delete_verts);
            std::cout << " took " << timer.get_elapsed() << "ms." << std::endl;
        }

        /* Check for color and delete if not existing. */
        mve::TriangleMesh::ColorList& colors = mesh->get_vertex_colors();
        if (!colors.empty() && colors[0].minimum() < 0.0f)
        {
            std::cout << "Removing dummy mesh coloring..." << std::endl;
            colors.clear();
        }

        {
            float threshold = 5.0f;
            std::cout << "Removing low-confidence geometry (threshold "
                      << threshold << ")..." << std::endl;
            std::size_t num_verts = mesh->get_vertices().size();
            mve::TriangleMesh::ConfidenceList const& confs = mesh->get_vertex_confidences();
            std::vector<bool> delete_list(confs.size(), false);
            for (std::size_t i = 0; i < confs.size(); ++i)
            {
                if (confs[i] > threshold)
                    continue;
                delete_list[i] = true;
            }
            mesh->delete_vertices_fix_faces(delete_list);
            std::size_t new_num_verts = mesh->get_vertices().size();
            std::cout << "  Deleted " << (num_verts - new_num_verts)
                      << " low-confidence vertices." << std::endl;
        }

        {
            int component_size = 1000;
            std::cout << "Removing isolated components below "
                      << component_size << " vertices..." << std::endl;
            std::size_t num_verts = mesh->get_vertices().size();
            mve::geom::mesh_components(mesh, component_size);
            std::size_t new_num_verts = mesh->get_vertices().size();
            std::cout << "  Deleted " << (num_verts - new_num_verts)
                      << " vertices in isolated regions." << std::endl;
        }

        {
            std::cout << "Removing degenerated faces..." << std::endl;
            std::size_t num_collapsed = fssr::clean_mc_mesh(mesh);
            std::cout << "  Collapsed " << num_collapsed << " edges." << std::endl;
        }
        /* Write output mesh. */
        mve::geom::SavePLYOptions ply_opts;
        ply_opts.write_vertex_colors = true;
        ply_opts.write_vertex_confidences = true;
        ply_opts.write_vertex_values = true;
        std::cout << "Mesh output file: " << "surface.ply" << std::endl;
        mve::geom::save_ply_mesh(mesh, mesh_name, ply_opts);
        std::cout << "Mesh reconstruction took " << total_timer.get_elapsed_sec() << "seconds."<< std::endl;
    } else {
        mesh = mve::geom::load_ply_mesh(mesh_name);
    }

    mve::TriangleMesh::VertexList &v_pos(mesh->get_vertices());
    mve::TriangleMesh::ColorList &v_color(mesh->get_vertex_colors());
    std::vector<Vertex> vertices(v_pos.size());
    glm::mat4 transform(1.0f);
    // inherit cluster's transform
    if (m_pCluster != nullptr) {
        transform = m_pCluster->GetTransform();
        m_pGLPanel->ClearObject(m_pCluster);
    }
    for (std::size_t i = 0; i < vertices.size(); ++i) {
        vertices[i].Position = glm::vec3(v_pos[i][0], v_pos[i][1], v_pos[i][2]);
        vertices[i].Color = glm::vec3(v_color[i][0], v_color[i][1], v_color[i][2]);
    }
    m_pCluster = m_pGLPanel->AddMesh(vertices, mesh->get_faces(), transform);
    Refresh();
    event.Skip();
}

void MainFrame::OnMenuDepthReconMVS(wxCommandEvent &event) {
    if (m_pScene == nullptr || m_pScene->get_views().empty()) {
        event.Skip();
        return;
    }
	std::string input_name;
    std::string dm_name;
    std::string ply_name = "point-set.ply";
    if (event.GetId() == MENU_DEPTH_RECON_MVS) {
        if (m_scale != 0)
            input_name = "undist-L" + std::to_string(m_scale);
        else
            input_name = "undistorted";
        dm_name = "depth-visual-L"
            + util::string::get(m_scale);
        ply_name = "point-set-visual.ply";
    } else if (event.GetId() == MENU_DEPTH_RECON_MVS_THERMAL) {
        input_name = "thermal";
        dm_name = "depth-thermal-L"
            + util::string::get(m_scale);
        ply_name = "point-set-thermal.ply";
    }
    util::WallTimer timer;
    mve::Scene::ViewList &views(m_pScene->get_views());
#pragma omp parallel for schedule(dynamic, 1)
    for (std::size_t id = 0; id < views.size(); ++id) {
        if (views[id] == nullptr || !views[id]->is_camera_valid())
            continue;

        if (!views[id]->has_image(input_name)) {
            if (!views[id]->has_image(UNDISTORTED_IMAGE_NAME)) {
                continue;
            } else if (m_scale != 0) {
                Util::resizeView(views[id], input_name, m_scale);
            }
        }

        /* Setup MVS. */
        mvs::Settings settings;
        settings.refViewNr = id;
        settings.imageEmbedding = input_name;
        settings.dmName = dm_name;

        if (views[id]->has_image(dm_name))
            continue;

        try {
            mvs::DMRecon recon(m_pScene, settings);
            recon.start();
        }
        catch (std::exception &err) {
            std::cerr << err.what() << std::endl;
        }
    }
    std::cout << "Reconstruction took "
              << timer.get_elapsed() << "ms." << std::endl;
    std::cout << "Saving views back to disc..." << std::endl;
    m_pScene->save_views();

    m_point_set = Util::GenerateMesh(m_pScene, input_name, dm_name, m_scale, ply_name);

    // display cluster
    mve::TriangleMesh::VertexList &v_pos(m_point_set->get_vertices());
    mve::TriangleMesh::ColorList &v_color(m_point_set->get_vertex_colors());
    std::vector<Vertex> vertices(v_pos.size());
    glm::mat4 transform(1.0f);
    if (!m_pGLPanel->GetTargetList().empty()) {
        for (const auto &obj : m_pGLPanel->GetTargetList()) {
            if (obj->As<Cluster>() != nullptr) {
                transform = obj->GetTransform();
                break;
            }
        }
    }
    if (m_pCluster != nullptr) {
        transform = m_pCluster->GetTransform();
        m_pGLPanel->ClearObject(m_pCluster);
    }
    for (std::size_t i = 0; i < vertices.size(); ++i) {
        vertices[i].Position = glm::vec3(v_pos[i][0], v_pos[i][1], v_pos[i][2]);
        vertices[i].Color = glm::vec3(v_color[i][0], v_color[i][1], v_color[i][2]);
    }
    m_pCluster = m_pGLPanel->AddCluster(vertices, transform);
    Refresh();

    event.Skip();
}

void MainFrame::OnMenuDisplayFrustum(wxCommandEvent &event) {
    m_pGLPanel->ClearObjects<Frustum>();
    if (event.IsChecked()) {
        for (const auto &view : m_pScene->get_views()) {
            if (!view->is_camera_valid())
                continue;

            glm::mat4 trans;
            view->get_camera().fill_cam_to_world(&trans[0].x);
            trans = Util::MveToGLMatrix(trans);
            if (m_pCluster != nullptr)
                trans = m_pCluster->GetTransform() * trans;
            m_pGLPanel->AddCameraFrustum(trans);
        }
    }
    Refresh();
    event.Skip();
}
