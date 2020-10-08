#ifndef _MAIN_FRAME_HPP
#define _MAIN_FRAME_HPP

#ifndef WX_PRECOMP

#include <wx/wxprec.h>

#endif

#include <wx/listctrl.h>
#include <vector>
#include "mve/view.h"
#include "mve/scene.h"

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
        MENU_DEPTH_RECON,
        MENU_PSET_RECON,
        MENU_FSS_RECON,
    };
private:
    void OnMenuOpenScene(wxCommandEvent &event);

    void OnMenuNewScene(wxCommandEvent &event);

    void OnMenuDoSfM(wxCommandEvent &event);

    void OnMenuDepthRecon(wxCommandEvent &event);

    ImageList m_originalImageList;

    wxMenuBar *m_pMenuBar;
    GLPanel *m_pGLPanel;
    mve::Scene::Ptr m_pScene;

    /** Display images owned by m_pScene on 'image_list' which have the name of 'image_name' */
    void DisplaySceneImage(const std::string &image_name, const ImageList &image_list);
};


#endif //_MAIN_FRAME_HPP
