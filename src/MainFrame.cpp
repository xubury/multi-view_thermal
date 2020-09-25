#include "MainFrame.hpp"
#include <cstring>
#include <algorithm>
#include <atomic>
#include "util/system.h"
#include "util/timer.h"
#include "util/file_system.h"
#include "mve/scene.h"
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

    m_pMenuBar->Append(pFileMenu, _("File"));
    this->SetMenuBar(m_pMenuBar);

    pFileMenu->Bind(wxEVT_MENU, &MainFrame::OnMenuOpenScene, this, MENU::MENU_SCENE_OPEN);
    pFileMenu->Bind(wxEVT_MENU, &MainFrame::OnMenuNewScene, this, MENU::MENU_SCENE_NEW);

    util::system::register_segfault_handler();
}

void MainFrame::OnMenuOpenScene(wxCommandEvent &event) {
    wxDirDialog dlg(this);
    if (dlg.ShowModal() == wxID_OK) {
        util::system::print_build_timestamp("Open MVE Scene");

        std::string aPath = dlg.GetPath().ToStdString();
        aPath = util::fs::sanitize_path(aPath);

        mve::Scene::Ptr scene;
        try {
            scene = mve::Scene::create(aPath);
        } catch (const std::exception &e) {
            std::cout << "Error Loading Scene: " << e.what() << std::endl;
            std::exit(EXIT_FAILURE);
        }
        std::cout << "View Size: " << scene->get_views().size() << std::endl;

        const std::string prebundle_path = util::fs::join_path(scene->get_path(), "prebundle.sfm");
        sfm::bundler::ViewportList viewPorts;
        sfm::bundler::PairwiseMatching pairwise_matching;
        if (!util::fs::file_exists(prebundle_path.c_str())) {

        } else {
            std::cout << "Loading pairwise matching from file..." << std::endl;
            sfm::bundler::load_prebundle_from_file(prebundle_path, &viewPorts, &pairwise_matching);
        }

        /* Drop descriptors and embeddings to save memory. */
        scene->cache_cleanup();
        for (auto & viewPort : viewPorts)
            viewPort.features.clear_descriptors();

        /* Check if there are some matching images. */
        if (pairwise_matching.empty()) {
            std::cerr << "No matching image pairs. Exiting." << std::endl;
            std::exit(EXIT_FAILURE);
        }

        /* Load camera intrinsics. */
        sfm::bundler::Intrinsics::Options intrinsics_opts;
        std::cout << "Initializing camera intrinsics..." << std::endl;
        sfm::bundler::Intrinsics intrinsics(intrinsics_opts);
        intrinsics.compute(scene, &viewPorts);
    }
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
            std::exit(EXIT_FAILURE);
        }
        std::cout << "Found " << dir.size() << " directory entries" << std::endl;


        std::sort(dir.begin(), dir.end());
        std::atomic_int id_cnt(0);
        std::atomic_int num_imported(0);
        m_views.clear();
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
            m_views.push_back(view);
        }
        std::cout << "Imported " << num_imported << " input images, took "
                  << timer.get_elapsed() << " ms." << std::endl;

        SetImageList("original", "ID");
    }
}

void MainFrame::SetImageList(const std::string &name, const std::string &label) {
    if (m_views.empty()) {
        return;
    }
    m_pImageListCtrl->DeleteAllItems();
    for (std::size_t i = 0; i < m_views.size(); ++i) {
        mve::ByteImage::Ptr image = m_views[i]->get_byte_image(name);
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

