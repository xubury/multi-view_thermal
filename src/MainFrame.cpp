#include "MainFrame.hpp"

MainFrame::MainFrame(wxWindow *parent, wxWindowID id, const wxString &title, const wxPoint &pos,
                     const wxSize &size) : wxFrame(parent, id, title, pos, size) {
    m_pMenuBar = new wxMenuBar();

    auto *pFileMenu = new wxMenu();
    pFileMenu->Append(MENU::MENU_SCENE_OPEN, _("Open Scene"));
    pFileMenu->Append(MENU::MENU_SCENE_NEW, _("New Scene"));

    m_pMenuBar->Append(pFileMenu, _("File"));
    this->SetMenuBar(m_pMenuBar);
}
