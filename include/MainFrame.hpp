#ifndef _MAIN_FRAME_HPP
#define _MAIN_FRAME_HPP

#include <wx/listctrl.h>
#include <wx/wx.h>

#include <vector>

#include "Cluster.hpp"
#include "mve/scene.h"
#include "mve/view.h"

class GLPanel;

class MainFrame : public wxFrame {
   public:
    /** ImageList, first is the list control display on window,
     * second is the list that holds icon data.*/
    typedef std::pair<wxListCtrl *, wxImageList *> ImageList;

   public:
    explicit MainFrame(wxWindow *parent, wxWindowID id = wxID_ANY,
                       const wxString &title = wxEmptyString,
                       const wxPoint &pos = wxDefaultPosition,
                       const wxSize &size = wxDefaultSize);

    enum MENU {
        MENU_SCENE_NEW,
        MENU_SCENE_OPEN,
        MENU_DO_SFM,
        MENU_DISPLAY_FRUSTUM,
        MENU_DEPTH_RECON_MVS,
        MENU_DEPTH_RECON_SHADING,
        MENU_DEPTH_RECON_SHADING_THERMAL,
        MENU_MESH_RECON_MVS,
        MENU_MESH_RECON_SHADING,
        MENU_FSS_RECON,
        MENU_GENERATE_DEPTH_IMG
    };

   private:
    void OnMenuOpenScene(wxCommandEvent &event);

    void OnMenuNewScene(wxCommandEvent &event);

    void OnMenuStructureFromMotion(wxCommandEvent &event);

    void OnMenuDisplayFrustum(wxCommandEvent &event);

    void OnMenuDepthReconShading(wxCommandEvent &event);

    void OnMenuDepthReconMVS(wxCommandEvent &event);

    void OnMenuMeshReconstruction(wxCommandEvent &event);

    void OnMenuFSSR(wxCommandEvent &event);

    /** Display images owned by m_pScene on 'image_list' which have the name of
     * 'image_name' */
    void DisplaySceneImage(const std::string &image_name,
                           const ImageList &image_list);

    ImageList m_originalImageList;

    GLPanel *m_pGLPanel;

    /** The pointer that hold all the data(image, death map...etc)*/
    mve::Scene::Ptr m_pScene;

    /** Image down scale factor for speeding up calculation */
    int m_scale;

    /** The pointer to OpenGL render target */
    RenderTarget::Ptr m_pCluster;

    /** The pointer to mvs construct result*/
    mve::TriangleMesh::Ptr m_point_set;

    void reconsturctSMVS(const std::string &input_name,
                         const std::string &dm_name, const std::string &sgmName);
};

#endif  //_MAIN_FRAME_HPP
