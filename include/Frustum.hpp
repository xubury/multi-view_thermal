#ifndef _FRUSTUM_HPP
#define _FRUSTUM_HPP

#include <vector>
#include "RenderTarget.hpp"

class Frustum : public RenderTarget {
public:
    explicit Frustum(const glm::mat4 &model);

    void SetFrustum(float nearZ, float farZ, float FOV, const glm::vec3 &color);

protected:
    void DrawArray() override;

private:
    unsigned int m_VAO;
    unsigned int m_VBO;
    std::vector<unsigned int> m_indices;
    std::vector<Vertex> m_vertices;
};


#endif //_FRUSTUM_HPP
