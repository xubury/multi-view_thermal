#include "RenderTarget.hpp"

RenderTarget::RenderTarget(const glm::mat4 &model) : m_model(model) {

}

void RenderTarget::Render(const Shader &shader, const Camera &camera) {
    shader.use();
    shader.setMat4f("projection", camera.GetPerspective());
    shader.setMat4f("view", camera.GetViewMatrix());
    shader.setMat4f("model", m_model);
    DrawArray();
}

void RenderTarget::Transform(const glm::mat4 &transform) {
    m_model = transform * m_model;
}

void RenderTarget::Rotate(const glm::mat4 &transform) {
    glm::vec4 pos = m_model[3];
    m_model[3] = glm::vec4(0.f, 0.f, 0.f, 1.f);
    m_model = transform * m_model;
    m_model[3] = pos;
}
