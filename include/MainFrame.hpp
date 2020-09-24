#ifndef _MAIN_FRAME_HPP
#define _MAIN_FRAME_HPP

#ifndef WX_PRECOMP
#include <wx/wxprec.h>
#endif
#include <wx/listctrl.h>
#include <vector>
#include "mve/view.h"

class MainFrame : public wxFrame {
public:
    explicit MainFrame(wxWindow *parent, wxWindowID id = wxID_ANY,
                       const wxString &title = wxEmptyString,
                       const wxPoint &pos = wxDefaultPosition,
                       const wxSize &size = wxDefaultSize);

    ~MainFrame() override = default;

    enum MENU {
        MENU_SCENE_NEW,
        MENU_SCENE_OPEN
    };
private:
    void OnMenuOpenScene(wxCommandEvent &event);

    void OnMenuNewScene(wxCommandEvent &event);

    wxListCtrl *m_pImageListCtrl;
    wxImageList *m_pImageList;
    wxMenuBar *m_pMenuBar;

    std::vector<mve::View::Ptr> m_views;

    void SetImageList(const std::string &name, const std::string &label);
};


#endif //_MAIN_FRAME_HPP
