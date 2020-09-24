#ifndef _MAIN_FRAME_HPP
#define _MAIN_FRAME_HPP

#ifndef WX_PRECOMP

#include <wx/wxprec.h>

#endif

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

    wxMenuBar *m_pMenuBar;
};


#endif //_MAIN_FRAME_HPP
