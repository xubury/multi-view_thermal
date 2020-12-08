#ifndef MESH_HPP
#define MESH_HPP

#include "RenderTarget.hpp"
#include <vector>

class Mesh : public RenderTarget {
public:
    using Ptr = std::shared_ptr<Mesh>;
public:
    explicit Mesh(const glm::mat4 &model);

    void SetMesh(const std::vector<Vertex> &vertices, const std::vector<unsigned int> &indices);
private:
    void DrawArray(const Shader &shader) override;

    unsigned int m_VAO;
    unsigned int m_VBO;
    unsigned int m_EBO;
    std::vector<Vertex> m_vertices;
    std::vector<int> m_indices;
};

#endif //MESH_HPP
