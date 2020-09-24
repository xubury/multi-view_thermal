#include "MainApp.hpp"

IMPLEMENT_APP(MainApp)

bool MainApp::OnInit() {
    auto *frame = new wxFrame(nullptr, wxID_ANY, "Multi-View Thermal Viewer");
    frame->Show();
    SetTopWindow(frame);
    return true;
}
