#ifndef _MAIN_FRAME_HPP
#define _MAIN_FRAME_HPP

#ifndef WX_PRECOMP

#include <wx/wxprec.h>

#endif

#include <wx/listctrl.h>
#include <vector>
#include "mve/view.h"
#include "mve/scene.h"
#include "Cluster.hpp"

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
        MENU_DEPTH_RECON_SHADING,
        MENU_DEPTH_RECON_MVS,

        MENU_DEPTH_TO_PSET,
        MENU_DEPTH_TO_PSET_THERMAL,
        MENU_FSS_RECON,


        MENU_GENERATE_DEPTH_IMG
    };
private:
    void OnMenuOpenScene(wxCommandEvent &event);

    void OnMenuNewScene(wxCommandEvent &event);

    void OnMenuStructureFromMotion(wxCommandEvent &event);

    void OnMenuDepthReconShading(wxCommandEvent &event);

    void OnMenuDepthReconMVS(wxCommandEvent &event);

    void OnMenuDepthToPointSet(wxCommandEvent &event);

    void OnMenuGenerateDepthImage(wxCommandEvent &event);

    void OnMenuFSSR(wxCommandEvent &event);

    /** Display images owned by m_pScene on 'image_list' which have the name of 'image_name' */
    void DisplaySceneImage(const std::string &image_name, const ImageList &image_list);

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
    void GenerateJPEGFromMVEI(const std::string &img_name);
};

#endif //_MAIN_FRAME_HPP
