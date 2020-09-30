#ifndef _GL_PANEL_HPP
#define _GL_PANEL_HPP

#include "RenderTarget.hpp"
#include "Camera.hpp"
#include <wx/wxprec.h>
#include <wx/glcanvas.h>
#include <vector>
#include <memory>
#include <glm/glm.hpp>

class Shader;

class GLPanel : public wxGLCanvas {
public:
    GLPanel(wxWindow *parent, wxWindowID win_id, int *displayAttrs = nullptr, const wxPoint &pos = wxDefaultPosition,
            const wxSize &size = wxDefaultSize, long style = 0, const wxString &name = wxGLCanvasName,
            const wxPalette &palette = wxNullPalette);

private:
    void OnRender(wxPaintEvent &event);

    void OnResize(wxSizeEvent &event);

    void OnMouseMove(wxMouseEvent &event);

    void OnMouseScroll(wxMouseEvent &event);

    std::unique_ptr<wxGLContext> m_pContext;

    std::unique_ptr<Shader> m_pShader;

    std::vector<RenderTarget::Ptr> m_targets;

    Camera::Ptr m_pCamera;

    wxPoint m_lastMouse;

    bool m_isFirstMouse;
};


#endif //_GL_PANEL_HPP
