#ifndef _GL_PANEL_HPP
#define _GL_PANEL_HPP

#include "RenderTarget.hpp"
#include "Axis.hpp"
#include "Frustum.hpp"
#include "Cluster.hpp"
#include "Camera.hpp"
#include <wx/wxprec.h>
#include <wx/glcanvas.h>
#include <vector>
#include <memory>
#include <algorithm>
#include <glm/glm.hpp>
#include <glbinding/glbinding.h>
#include "Mesh.hpp"

class GLPanel : public wxGLCanvas {
public:
    GLPanel(wxWindow *parent, wxWindowID win_id, int *displayAttrs = nullptr, const wxPoint &pos = wxDefaultPosition,
            const wxSize &size = wxDefaultSize, long style = 0, const wxString &name = wxGLCanvasName,
            const wxPalette &palette = wxNullPalette);

    void AddCameraFrustum(const glm::mat4 &transform);

    Cluster::Ptr AddCluster(const std::vector<Vertex> &vertices,
                            const glm::mat4 &transform = glm::mat4(1.0f));

    Mesh::Ptr AddMesh(const std::vector<Vertex> &vertices, const std::vector<unsigned int> &indices,
                            const glm::mat4 &transform = glm::mat4(1.0f));
    template<typename T>
    void ClearObjects();

    void ClearObject(const RenderTarget::Ptr &target);

    static void OpenGLDebugMessage(gl::GLenum source, gl::GLenum type, gl::GLuint id, gl::GLenum severity,
                                   gl::GLsizei length, const gl::GLchar *message, const void *userdata);

    std::vector<RenderTarget::Ptr> GetTargetList();
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

template<typename T>
inline void GLPanel::ClearObjects() {
    m_targets.erase(std::remove_if(m_targets.begin(), m_targets.end(), [](const auto &target) -> bool {
      return dynamic_cast<T *>(target.get()) != nullptr;
    }), m_targets.end());
}

inline void GLPanel::ClearObject(const RenderTarget::Ptr &target) {
    if (target == nullptr) return;
    m_targets.erase(std::remove_if(m_targets.begin(), m_targets.end(), [target](const auto &v) { return target == v; }),
                    m_targets.end());
}

inline std::vector<RenderTarget::Ptr> GLPanel::GetTargetList() {
    return m_targets;
}

#endif //_GL_PANEL_HPP
