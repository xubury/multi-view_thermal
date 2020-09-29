#include "RenderTarget.hpp"

RenderTarget::RenderTarget(const glm::mat4 &model) : m_model(model){

}

void RenderTarget::Render(const Shader &shader, const glm::mat4 &projection, const glm::mat4 &view) {
    shader.use();
    shader.setMat4f("projection", projection);
    shader.setMat4f("view", view);
    shader.setMat4f("model", m_model);
    DrawArray();
}

