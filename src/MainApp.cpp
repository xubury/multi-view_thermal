#include "MainApp.hpp"
#include "MainFrame.hpp"

IMPLEMENT_APP(MainApp)

bool MainApp::OnInit() {
    auto *frame = new MainFrame(nullptr, wxID_ANY, _("Multi-View Thermal Viewer"),
                                wxDefaultPosition, wxSize(1024, 768));
    frame->Show();
    SetTopWindow(frame);
    return true;
}
