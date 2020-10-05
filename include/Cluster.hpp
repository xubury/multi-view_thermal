#ifndef _CLUSTER_HPP
#define _CLUSTER_HPP

#include "RenderTarget.hpp"

class Cluster : public RenderTarget {
public:
    using Ptr = std::shared_ptr<Cluster>;
public:
    explicit Cluster(const glm::mat4 &model);

    void SetCluster(const std::vector<Vertex> &vertices);

private:
    void DrawArray() override;

    unsigned int m_VAO;
    unsigned int m_VBO;
    std::vector<Vertex> m_vertices;
};


#endif //_CLUSTER_HPP
