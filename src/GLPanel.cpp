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
