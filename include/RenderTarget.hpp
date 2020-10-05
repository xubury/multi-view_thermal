#ifndef _RENDER_TARGET_HPP
#define _RENDER_TARGET_HPP

#include <memory>
#include "Shader.hpp"
#include "Camera.hpp"

struct Vertex {
    glm::vec3 Position;
    glm::vec3 Color;

    Vertex(const glm::vec3 &pos = glm::vec3(0), const glm::vec3 &color = glm::vec3(1)) : Position(pos), Color(color) {}
};

class RenderTarget {
public:
    typedef std::shared_ptr<RenderTarget> Ptr;

    template<typename T>
    static Ptr Create(const glm::mat4 &model = glm::mat4(1.0));

    template<typename T>
    T *As();

public:
    explicit RenderTarget(const glm::mat4 &model);

    void Render(const Shader &shader, const Camera &camera);

    void Transform(const glm::mat4 &transform);

    glm::vec3 GetPosition();

    virtual void DrawArray() = 0;

private:
    glm::mat4 m_model;
};

template<typename T>
inline RenderTarget::Ptr RenderTarget::Create(const glm::mat4 &model) {
    static_assert(std::is_base_of<RenderTarget, T>::value, "T must be derived from RenderTarget");
    RenderTarget::Ptr target(new T(model));
    return target;
}

template<typename T>
T *RenderTarget::As() {
    static_assert(std::is_base_of<RenderTarget, T>::value, "T must be derived from RenderTarget");
    return dynamic_cast<T *>(this);
}

inline glm::vec3 RenderTarget::GetPosition() {
    return m_model[3];
}

#endif //_RENDER_TARGET_HPP
