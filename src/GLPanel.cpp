#include "GLPanel.hpp"
#include <glbinding/glbinding.h>

GLPanel::GLPanel(wxWindow *parent, wxWindowID win_id, int *displayAttrs,
                 const wxPoint &pos, const wxSize &size, long style,
                 const wxString &name, const wxPalette &palette) : wxGLCanvas(parent, win_id, displayAttrs, pos, size,
                                                                              style, name, palette) {
    m_pContext = std::make_unique<wxGLContext>(this);
    this->SetCurrent(*m_pContext);

    glbinding::initialize(nullptr);

    this->Bind(wxEVT_PAINT, &GLPanel::OnRender, this);
    this->Bind(wxEVT_SIZE, &GLPanel::OnResize, this);
}

void GLPanel::OnRender(wxPaintEvent &) {
    if (!IsShown())
        return;
    SetCurrent(*m_pContext);
    wxPaintDC(this);
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    SwapBuffers();
}

void GLPanel::OnResize(wxSizeEvent &event)
{
    glViewport(0, 0, GetSize().GetWidth(), GetSize().GetHeight());
    Update();
    event.Skip();
}