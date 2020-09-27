#include "MainFrame.hpp"
#include <cstring>
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
#include "Image.hpp"

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
    pBoxSizer->Add(pImageListCtrl, 0, wxEXPAND);
    this->SetSizerAndFit(pBoxSizer);

    m_pMenuBar = new wxMenuBar();

    auto *pFileMenu = new wxMenu();
    pFileMenu->Append(MENU::MENU_SCENE_OPEN, _("Open Scene"));
    pFileMenu->Append(MENU::MENU_SCENE_NEW, _("New Scene"));
    pFileMenu->Append(MENU::MENU_DO_SFM, _("Do Sfm"));

    m_pMenuBar->Append(pFileMenu, _("File"));
    this->SetMenuBar(m_pMenuBar);
    this->CreateStatusBar(1);
    pFileMenu->Bind(wxEVT_MENU, &MainFrame::OnMenuOpenScene, this, MENU::MENU_SCENE_OPEN);
    pFileMenu->Bind(wxEVT_MENU, &MainFrame::OnMenuNewScene, this, MENU::MENU_SCENE_NEW);
    pFileMenu->Bind(wxEVT_MENU, &MainFrame::OnMenuDoSfM, this, MENU::MENU_DO_SFM);

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
    mve::Scene::ViewList& views = m_pScene->get_views();
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
    for (int i = 0; i < listCtrl->GetColumnCount(); ++i)
        listCtrl->SetColumnWidth(i, -1);
}

void MainFrame::OnMenuDoSfM(wxCommandEvent &event) {
    // prebundle.sfm is for holding view and view matching info
    const std::string prebundle_path = util::fs::join_path(m_pScene->get_path(), "prebundle.sfm");
    sfm::bundler::ViewportList viewPorts;
    sfm::bundler::PairwiseMatching pairwise_matching;
    if (!util::fs::file_exists(prebundle_path.c_str())) {
        std::cout << "Start feature matching." << std::endl;
        util::system::rand_seed(RAND_SEED_MATCHING);
        if (!features_and_matching(m_pScene, &viewPorts, &pairwise_matching))
            return; // no feature match

        std::cout << "Saving pre-bundle to file..." << std::endl;
        sfm::bundler::save_prebundle_to_file(
                viewPorts, pairwise_matching, prebundle_path);
    } else {
        std::cout << "Loading pairwise matching from file..." << std::endl;
        sfm::bundler::load_prebundle_from_file(prebundle_path, &viewPorts, &pairwise_matching);
    }

    /* Drop descriptors and embeddings to save memory. */
    m_pScene->cache_cleanup();
    for (auto & viewPort : viewPorts)
        viewPort.features.clear_descriptors();

    /* Check if there are some matching images. */
    if (pairwise_matching.empty()) {
        std::cerr << "No matching image pairs." << std::endl;
        return;
    }

    /* Load camera intrinsics. */
    {
        sfm::bundler::Intrinsics::Options intrinsics_opts;
        if (m_pScene->get_views().back()->has_blob("exif"))
        {
            //get camera intrinsics from exif
            std::cout << "Initializing camera intrinsics from exif..." << std::endl;
            intrinsics_opts.intrinsics_source = sfm::bundler::Intrinsics::FROM_EXIF;
        }
        else
        {
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
    for (auto & viewPort : viewPorts) {
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

    event.Skip();
}

