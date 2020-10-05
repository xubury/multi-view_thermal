#include "Cluster.hpp"

using namespace gl;

Cluster::Cluster(const glm::mat4 &model) : RenderTarget(model), m_VAO(0), m_VBO(0) {

    glGenVertexArrays(1, &m_VAO);
    glBindVertexArray(m_VAO);

    glGenBuffers(1, &m_VBO);
    glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex) * m_vertices.size(), m_vertices.data(), GL_STATIC_DRAW);


    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void *) nullptr);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void *) offsetof(Vertex, Color));

    glBindVertexArray(0);
}

void Cluster::DrawArray() {
    glBindVertexArray(m_VAO);
    glDrawArrays(GL_POINTS, 0, m_vertices.size());
    glBindVertexArray(0);
}

void Cluster::SetCluster(const std::vector<Vertex> &vertices) {
    m_vertices.resize(vertices.size());
    std::copy(vertices.begin(), vertices.end(), m_vertices.data());
    glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex) * m_vertices.size(), m_vertices.data(), GL_STATIC_DRAW);
}
