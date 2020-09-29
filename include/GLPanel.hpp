#ifndef _GL_PANEL_HPP
#define _GL_PANEL_HPP

#include <wx/wxprec.h>
#include <wx/glcanvas.h>
#include <memory>

class Shader;

class GLPanel : public wxGLCanvas {
public:
    GLPanel(wxWindow *parent, wxWindowID win_id, int *displayAttrs = nullptr, const wxPoint &pos = wxDefaultPosition,
            const wxSize &size = wxDefaultSize, long style = 0, const wxString &name = wxGLCanvasName,
            const wxPalette &palette = wxNullPalette);

    ~GLPanel() override = default;

private:
    void OnRender(wxPaintEvent &event);

    void OnResize(wxSizeEvent &event);

    std::unique_ptr<wxGLContext> m_pContext;

    std::unique_ptr<Shader> m_pShader;
};


#endif //_GL_PANEL_HPP
