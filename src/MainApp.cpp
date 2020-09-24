#include "MainApp.hpp"
#include "MainFrame.hpp"

IMPLEMENT_APP(MainApp)

bool MainApp::OnInit() {
    auto *frame = new MainFrame(nullptr, wxID_ANY, _("Multi-View Thermal Viewer"));
    frame->Show();
    SetTopWindow(frame);
    return true;
}
