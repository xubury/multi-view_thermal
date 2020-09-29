#include "Camera.hpp"

Camera::Camera(const glm::vec3 &position, const glm::vec3 &up, float yaw, float pitch)
        : m_position(position), m_front(glm::vec3(0.f, 0.f, -1.f)),
          m_up(0), m_right(0), m_worldUp(up), m_yaw(yaw),
          m_pitch(pitch), m_movementSpeed(SPEED), m_mouseSensitivity(SENSITIVITY), m_zoom(ZOOM) {
    UpdateCameraVectors();
}

glm::mat4 Camera::GetViewMatrix() const {
    return glm::lookAt(m_position, m_position + m_front, m_up);
}

void Camera::ProcessKeyboard(Camera_Movement direction, float deltatime) {
    float velocity = m_movementSpeed * deltatime;
    if (direction == FORWARD) {
        m_position += m_front * velocity;
    } else if (direction == BACKWARD) {
        m_position -= m_front * velocity;
    } else if (direction == LEFT) {
        m_position -= m_right * velocity;
    } else if (direction == RIGHT) {
        m_position += m_right * velocity;
    } else if (direction == UP) {
        m_position += m_up * velocity;
    } else if (direction == DOWN) {
        m_position -= m_up * velocity;
    } else if (direction == LEFT_ROTATE) {
        m_yaw += 0.5f;
        UpdateCameraVectors();
        m_position -= m_right * velocity;
    } else if (direction == RIGHT_ROTATE) {
        m_yaw -= 0.5f;
        UpdateCameraVectors();
        m_position += m_right * velocity;
    }
}

void Camera::ProcessMouseMovement(float xoffset, float yoffset, gl::GLboolean constraintPitch) {
    xoffset *= m_mouseSensitivity;
    yoffset *= m_mouseSensitivity;

    m_yaw += xoffset;
    m_pitch += yoffset;

    if (constraintPitch) {
        if (m_pitch > 89.f) {
            m_pitch = 89.f;
        } else if (m_pitch < -89.f) {
            m_pitch = -89.f;
        }
    }
    UpdateCameraVectors();
}

void Camera::ProcessMouseScroll(float yoffset) {
    m_zoom -= yoffset;
    if (m_zoom < 1.0f) {
        m_zoom = 1.0f;
    } else if (m_zoom > 45.f) {
        m_zoom = 45.f;
    }
}

void Camera::UpdateCameraVectors() {
    // calculate the new Front vector
    glm::vec3 front;
    front.x = cos(glm::radians(m_yaw)) * cos(glm::radians(m_pitch));
    front.y = sin(glm::radians(m_pitch));
    front.z = sin(glm::radians(m_yaw)) * cos(glm::radians(m_pitch));
    m_front = glm::normalize(front);
    m_right = glm::normalize(glm::cross(m_front, m_worldUp));
    m_up = glm::normalize(glm::cross(m_right, m_front));
}



