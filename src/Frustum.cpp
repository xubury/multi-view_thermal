#include "Frustum.hpp"
#include "Axis.hpp"
#include "glbinding/gl/gl.h"

using namespace gl;

Frustum::Frustum(const glm::mat4 &model) : RenderTarget(model), m_VAO(0), m_VBO(0),
                                           m_pAxis(std::make_unique<Axis>(model)) {
    m_indices = {
        0, 1, 1, 2, 2, 3, 3, 0,
        4, 5, 5, 6, 6, 7, 7, 4,
        0, 4, 1, 5, 2, 6, 3, 7
    };

    glGenVertexArrays(1, &m_VAO);
    glBindVertexArray(m_VAO);

    glGenBuffers(1, &m_VBO);
    glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex) * m_vertices.size(), m_vertices.data(), GL_STATIC_DRAW);

    unsigned int EBO = 0;
    glGenBuffers(1, &EBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int) * m_indices.size(), m_indices.data(), GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *) nullptr);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *) offsetof(Vertex, Color));

    glBindVertexArray(0);

    SetFrustum(0, 1, 45, glm::vec3(0.8f));
}

void Frustum::DrawArray() {
    glBindVertexArray(m_VAO);
    glDrawElements(GL_LINES, m_indices.size(), GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);
    m_pAxis->As<RenderTarget>()->DrawArray();
}

void Frustum::SetFrustum(float nearZ, float farZ, float FOV, const glm::vec3 &color) {
    float nearX = nearZ * tan(glm::radians(FOV) / 2);
    float nearY = nearX;
    float farX = farZ * tan(glm::radians(FOV) / 2);
    float farY = farX;
    m_vertices = {
        Vertex(glm::vec3(nearX, nearY, nearZ), color),
        Vertex(glm::vec3(-nearX, nearY, nearZ), color),
        Vertex(glm::vec3(-nearX, -nearY, nearZ), color),
        Vertex(glm::vec3(nearX, -nearY, nearZ), color),
        Vertex(glm::vec3(farX, farY, farZ), color),
        Vertex(glm::vec3(-farX, farY, farZ), color),
        Vertex(glm::vec3(-farX, -farY, farZ), color),
        Vertex(glm::vec3(farX, -farY, farZ), color)
    };
    glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex) * m_vertices.size(), m_vertices.data(), GL_STATIC_DRAW);
}
