#ifndef _CAMERA_H
#define _CAMERA_H

#include <glbinding/gl/gl.h>
#include <glbinding/glbinding.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <memory>

enum Camera_Movement {
    FORWARD,
    BACKWARD,
    UP,
    DOWN,
    LEFT,
    RIGHT,
    LEFT_ROTATE,
    RIGHT_ROTATE
};

constexpr float YAW = -90.0f;
constexpr float PITCH = 0.0f;
constexpr float SPEED = 2.5f;
constexpr float SENSITIVITY = 0.1f;
constexpr float ZOOM = 45.f;

class Camera {
public:
    typedef std::unique_ptr<Camera> Ptr;

    static Ptr
    Create(const glm::vec3 &position, const glm::vec3 &up = glm::vec3(0.f, 1.f, 0.f), float yaw = YAW, float pitch = PITCH);

    enum class Type {
        ORTHODOX,
        PERSPECTIVE
    };

public:
    explicit Camera(const glm::vec3 &position, const glm::vec3 &up = glm::vec3(0.f, 1.f, 0.f), float yaw = YAW, float pitch = PITCH);

    glm::mat4 GetViewMatrix() const;

    glm::vec3 GetPosition() const;

    float GetFOV() const;

    void ProcessKeyboard(Camera_Movement direction, float deltatime);

    void ProcessMouseMovement(float xoffset, float yoffset, gl::GLboolean constraintPitch = true);

    void ProcessMouseScroll(float yoffset);

    glm::vec3 GetRight();

    glm::vec3 GetUp();

    void SetScreenSize(int width, int height);

    glm::mat4 GetProjection() const;

    glm::mat4 CalculateRotateFromView(float x_offset, float y_offset) const;

    glm::mat4 CalculateTranslateFromView(float x_offset, float y_offset) const;

    void SetCameraType(Type type);
private:
    void UpdateCameraVectors();

    glm::vec3 m_position;
    glm::vec3 m_front;
    glm::vec3 m_up;
    glm::vec3 m_right;
    glm::vec3 m_worldUp;

    float m_yaw;
    float m_pitch;
    float m_movementSpeed;
    float m_mouseSensitivity;
    float m_zoom;

    int m_screenWidth;
    int m_screenHeight;

    Type m_type;
};

inline Camera::Ptr Camera::Create(const glm::vec3 &position, const glm::vec3 &up, float yaw, float pitch) {
    Camera::Ptr camera(new Camera(position, up, yaw, pitch));
    return camera;
}

inline glm::vec3 Camera::GetRight() {
    return m_right;
}

inline glm::vec3 Camera::GetUp() {
    return m_up;
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

#endif //_CAMERA_HPP
