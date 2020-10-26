#ifndef _CAMERA_H
#define _CAMERA_H

#include <glbinding/gl/gl.h>
#include <glbinding/glbinding.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <memory>

constexpr float ZOOM = 45.f;

class Camera {
public:
    typedef std::unique_ptr<Camera> Ptr;

    static Camera::Ptr
    Create(const glm::vec3 &position,
           const glm::vec3 &front = glm::vec3(0, 0, -1),
           const glm::vec3 &up = glm::vec3(0, 1, 0));

    enum class Type {
        ORTHODOX,
        PERSPECTIVE
    };

public:
    explicit Camera(const glm::vec3 &position, const glm::vec3 &front, const glm::vec3 &up);

    glm::mat4 GetViewMatrix() const;

    glm::vec3 GetPosition() const;

    float GetFOV() const;

    void ProcessMouseScroll(float yoffset);

    void SetScreenSize(int width, int height);

    glm::mat4 GetProjection() const;

    glm::mat4 CalculateRotateFromView(float x_offset, float y_offset) const;

    glm::mat4 CalculateTranslateFromView(float x_offset, float y_offset) const;

    void SetCameraType(Type type);

    int GetScreenWidth() const;

    int GetScreenHeight() const;

private:
    glm::vec3 m_position;
    glm::vec3 m_front;
    glm::vec3 m_up;

    float m_zoom;

    int m_screenWidth;
    int m_screenHeight;

    Type m_type;
};

inline Camera::Ptr Camera::Create(const glm::vec3 &position, const glm::vec3 &front, const glm::vec3 &up) {
    Camera::Ptr camera(new Camera(position, front, up));
    return camera;
}

inline glm::vec3 Camera::GetPosition() const {
    return m_position;
}

inline float Camera::GetFOV() const {
    return m_zoom;
}

inline void Camera::SetScreenSize(int width, int height) {
    m_screenWidth = width;
    m_screenHeight = height;
}

inline void Camera::SetCameraType(Type type) {
    m_type = type;
}

inline int Camera::GetScreenHeight() const {
    return m_screenHeight;
}

inline int Camera::GetScreenWidth() const {
    return m_screenWidth;
}

#endif //_CAMERA_HPP
