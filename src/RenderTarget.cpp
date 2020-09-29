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

void RenderTarget::RotateFromView(float x_offset, float y_offset, const glm::mat4 &view) {
    glm::mat4 rotation(1);
    rotation = glm::rotate(rotation, glm::radians((float) x_offset), (glm::vec3)view[1]);
    rotation = glm::rotate(rotation, glm::radians((float) -y_offset), (glm::vec3)view[0]);
    m_model = glm::translate(glm::mat4(1), glm::vec3(m_model[3])) * rotation
           * glm::translate(glm::mat4(1), -glm::vec3(m_model[3])) * m_model;
}

void RenderTarget::TranslateFromView(float x_offset, float y_offset, const glm::mat4 &projection, const glm::mat4 &view,
                             int clip_width, int clip_height) {
    float xOffset_clip = (2 * x_offset) / (float)clip_width;
    float yOffset_clip = (2 * y_offset) / (float)clip_height;
    glm::mat4 transform = glm::translate(glm::mat4(1.0), glm::vec3(xOffset_clip, yOffset_clip, 0));
    transform = glm::inverse(projection * view) * transform * projection * view;
    float x = transform[3][0];
    float y = transform[3][1];
    float z = transform[3][2];
    transform = glm::translate(glm::mat4(1.0f), glm::vec3(x, y, z));
    m_model = transform * m_model;
}
