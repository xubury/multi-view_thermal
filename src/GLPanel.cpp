#include "GLPanel.hpp"
#include "Shader.hpp"
#include "Axis.hpp"

GLPanel::GLPanel(wxWindow *parent, wxWindowID win_id, int *displayAttrs,
                 const wxPoint &pos, const wxSize &size, long style,
                 const wxString &name, const wxPalette &palette) : wxGLCanvas(parent, win_id, displayAttrs, pos, size,
                                                                              style, name, palette),
                                                                   m_projection(1.0f) {
    m_pContext = std::make_unique<wxGLContext>(this);
    this->SetCurrent(*m_pContext);

    glbinding::initialize(nullptr);

    m_pShader = Shader::Create("vertex.glsl", "fragment.glsl");
    m_targets.emplace_back(RenderTarget::Create<Axis>());

    m_pCamera = Camera::Create(glm::vec3(0, 0, 3.f));

    this->Bind(wxEVT_PAINT, &GLPanel::OnRender, this);
    this->Bind(wxEVT_SIZE, &GLPanel::OnResize, this);
    this->Bind(wxEVT_MOTION, &GLPanel::OnMouseMove, this);
}

void GLPanel::OnRender(wxPaintEvent &) {
    if (!IsShown())
        return;
    SetCurrent(*m_pContext);
    wxPaintDC(this);
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glm::mat4 view = m_pCamera->GetViewMatrix();
    for (const auto &target : m_targets) {
        target->Render(*m_pShader, m_projection, view);
    }
    SwapBuffers();
}

void GLPanel::OnResize(wxSizeEvent &event) {
    glViewport(0, 0, GetSize().GetWidth(), GetSize().GetHeight());
    m_projection = glm::perspective(glm::radians(m_pCamera->GetFOV()),
                                    (float) GetSize().GetWidth() / (float) GetSize().GetHeight(),
                                    0.1f, 100.0f);
    Refresh();
    event.Skip();
}

void GLPanel::OnMouseMove(wxMouseEvent &event) {
    if (m_isFirstMouse)
    {
        m_lastMouse = event.GetPosition();
        m_isFirstMouse = false;
    }
    wxPoint mouse = event.GetPosition();
    float x_offset = (float)mouse.x - (float)m_lastMouse.x;
    float y_offset = (float)m_lastMouse.y - (float)mouse.y;
    for (auto &target : m_targets) {
        if(event.ButtonIsDown(wxMOUSE_BTN_LEFT)) {
            target->RotateFromView(x_offset, y_offset, m_pCamera->GetViewMatrix());
        }
        else if (event.ButtonIsDown(wxMOUSE_BTN_RIGHT)) {
            target->TranslateFromView(x_offset, y_offset, m_projection, m_pCamera->GetViewMatrix(),
                              GetSize().GetWidth(), GetSize().GetHeight());
        }
    }
    m_lastMouse = event.GetPosition();
    Refresh();
    event.Skip();
}
