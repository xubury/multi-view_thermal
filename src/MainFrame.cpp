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
#include "Image.hpp"

MainFrame::MainFrame(wxWindow *parent, wxWindowID id, const wxString &title, const wxPoint &pos,
                     const wxSize &size) : wxFrame(parent, id, title, pos, size),
                                           m_pImageList(nullptr) {
    wxInitAllImageHandlers();
    m_pImageListCtrl = new wxListCtrl(this);
    m_pImageList = new wxImageList(128, 128, false);
    m_pImageListCtrl->SetImageList(m_pImageList, wxIMAGE_LIST_NORMAL);
    m_pMenuBar = new wxMenuBar();

    auto *pFileMenu = new wxMenu();
    pFileMenu->Append(MENU::MENU_SCENE_OPEN, _("Open Scene"));
    pFileMenu->Append(MENU::MENU_SCENE_NEW, _("New Scene"));
    pFileMenu->Append(MENU::MENU_DO_SFM, _("Do Sfm"));

    m_pMenuBar->Append(pFileMenu, _("File"));
    this->SetMenuBar(m_pMenuBar);

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
                view->set_image_ref(afname, "original");
            else
                view->set_image(image, "original");

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
        SetImageList("original", "ID");
    }
    event.Skip();
}

void MainFrame::SetImageList(const std::string &name, const std::string &label) {
    mve::Scene::ViewList& views = m_pScene->get_views();
    if (views.empty()) {
        return;
    }
    m_pImageListCtrl->DeleteAllItems();
    for (std::size_t i = 0; i < views.size(); ++i) {
        mve::ByteImage::Ptr image = views[i]->get_byte_image(name);
        wxImage icon(image->width(), image->height());
        memcpy(icon.GetData(), image->get_data_pointer(), image->get_byte_size());
        int width = 0;
        int height = 0;
        m_pImageList->GetSize(i, width, height);
        icon.Rescale(width, height);
        if (!m_pImageList->Replace(i, icon)) {
            m_pImageList->Add(icon);
        }
        m_pImageListCtrl->InsertItem(i, wxString::Format("%s %lld", label, i), i);
    }
}

void MainFrame::OnMenuDoSfM(wxCommandEvent &event) {
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
    sfm::bundler::Intrinsics::Options intrinsics_opts;
    std::cout << "Initializing camera intrinsics..." << std::endl;
    sfm::bundler::Intrinsics intrinsics(intrinsics_opts);
    intrinsics.compute(m_pScene, &viewPorts);
    event.Skip();
}

