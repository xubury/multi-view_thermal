#include "GLPanel.hpp"
#include "Shader.hpp"
#include "Axis.hpp"
#include "Frustum.hpp"
#include <algorithm>

GLPanel::GLPanel(wxWindow *parent, wxWindowID win_id, int *displayAttrs,
                 const wxPoint &pos, const wxSize &size, long style,
                 const wxString &name, const wxPalette &palette) : wxGLCanvas(parent, win_id, displayAttrs, pos, size,
                                                                              style, name, palette),
                                                                   m_isFirstMouse(true) {
    m_pContext = std::make_unique<wxGLContext>(this);
    this->SetCurrent(*m_pContext);

    glbinding::initialize(nullptr);

    m_pShader = Shader::Create("vertex.glsl", "fragment.glsl");
    m_targets.emplace_back(RenderTarget::Create<Axis>());
    glm::mat4 trans(1.0);
    trans = glm::translate(trans, glm::vec3(0, 0, 1));
    RenderTarget::Ptr frustum = RenderTarget::Create<Frustum>(trans);
    m_targets.emplace_back(frustum);

    m_pCamera = Camera::Create(glm::vec3(0, 0, 45.f));
    m_pCamera->SetCameraType(Camera::Type::ORTHODOX);
    m_pCamera->SetScreenSize(GetSize().GetWidth(), GetSize().GetHeight());

    Bind(wxEVT_PAINT, &GLPanel::OnRender, this);
    Bind(wxEVT_SIZE, &GLPanel::OnResize, this);
    Bind(wxEVT_MOTION, &GLPanel::OnMouseMove, this);
    Bind(wxEVT_MOUSEWHEEL, &GLPanel::OnMouseScroll, this);
}

void GLPanel::OnRender(wxPaintEvent &) {
    if (!IsShown())
        return;
    SetCurrent(*m_pContext);
    wxPaintDC(this);
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    for (const auto &target : m_targets) {
        target->Render(*m_pShader, *m_pCamera);
    }
    SwapBuffers();
}

void GLPanel::OnResize(wxSizeEvent &event) {
    glViewport(0, 0, GetSize().GetWidth(), GetSize().GetHeight());
    m_pCamera->SetScreenSize(GetSize().GetWidth(), GetSize().GetHeight());
    Refresh();
    event.Skip();
}

void GLPanel::OnMouseMove(wxMouseEvent &event) {
    if (m_isFirstMouse) {
        m_lastMouse = event.GetPosition();
        m_isFirstMouse = false;
    }
    wxPoint mouse = event.GetPosition();
    float x_offset = (float) mouse.x - (float) m_lastMouse.x;
    float y_offset = (float) m_lastMouse.y - (float) mouse.y;
    glm::mat4 transform(1.0f);
    if (event.ButtonIsDown(wxMOUSE_BTN_LEFT)) {
        transform = m_pCamera->CalculateRotateFromView(x_offset, y_offset);
    } else if (event.ButtonIsDown(wxMOUSE_BTN_RIGHT)) {
        transform = m_pCamera->CalculateTranslateFromView(x_offset, y_offset);
    }
    for (auto &target : m_targets) {
        target->Transform(transform);
    }
    m_lastMouse = event.GetPosition();
    Refresh();
    event.Skip();
}

void GLPanel::OnMouseScroll(wxMouseEvent &event) {
    float scroll = (float) event.GetWheelRotation() / 50;
    m_pCamera->ProcessMouseScroll(scroll);
    Refresh();
    event.Skip();
}

void GLPanel::AddCameraFrustum(const glm::mat4 &transform) {
    RenderTarget::Ptr frustum = RenderTarget::Create<Frustum>(transform);
    m_targets.emplace_back(frustum);
}

void GLPanel::ClearCameraFrustum() {
    m_targets.erase(std::remove_if(m_targets.begin(), m_targets.end(), [](const auto &target)->bool {
        return dynamic_cast<Frustum *>(target.get()) != nullptr;
    }), m_targets.end());
}