#include "RenderTarget.hpp"

RenderTarget::RenderTarget(const glm::mat4 &model) : m_model(model) {

}

void RenderTarget::Render(const Shader &shader, const Camera &camera) {
    shader.use();
    shader.setMat4f("projection", camera.GetProjection());
    shader.setMat4f("view", camera.GetViewMatrix());
    shader.setMat4f("model", m_model);
    DrawArray();
}

void RenderTarget::Transform(const glm::mat4 &transform) {
    m_model = transform * m_model;
}
