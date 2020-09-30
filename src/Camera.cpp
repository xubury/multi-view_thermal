#include "Camera.hpp"

Camera::Camera(const glm::vec3 &position, const glm::vec3 &up, float yaw, float pitch)
        : m_position(position), m_front(glm::vec3(0.f, 0.f, -1.f)),
          m_up(0), m_right(0), m_worldUp(up), m_yaw(yaw),
          m_pitch(pitch), m_movementSpeed(SPEED), m_mouseSensitivity(SENSITIVITY), m_zoom(ZOOM),
          m_screenWidth(0), m_screenHeight(0), m_type(Type::PERSPECTIVE) {
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

glm::mat4 Camera::GetProjection() const {
    if (m_type == Type::PERSPECTIVE)
        return glm::perspective(glm::radians(GetFOV()), (float)m_screenWidth / (float)m_screenHeight, 0.1f, 100.0f);
    else {
        glm::mat4 view = GetViewMatrix();
        float ratio_size_per_depth = atan(glm::radians(GetFOV() / 2.0f)) * 2.0f;
        float xFactor = (float) m_screenWidth / (float) (m_screenWidth + m_screenHeight) * abs(view[3][2]) * ratio_size_per_depth;
        float yFactor = (float) m_screenHeight / (float) (m_screenWidth + m_screenHeight) * abs(view[3][2]) * ratio_size_per_depth;
        return glm::ortho(-xFactor, xFactor, -yFactor, yFactor, 0.1f, 100.0f);
    }
}

glm::mat4 Camera::CalculateRotateFromView(float x_offset, float y_offset) const {
    glm::mat4 rotation(1);
    glm::mat4 view = GetViewMatrix();
    rotation = glm::rotate(rotation, glm::radians((float) x_offset), glm::vec3(view[1]));
    rotation = glm::rotate(rotation, glm::radians((float) -y_offset), glm::vec3(view[0]));
    return rotation;
}

glm::mat4 Camera::CalculateTranslateFromView(float x_offset, float y_offset) const {
    glm::mat4 projection = GetProjection();
    glm::mat4 view = GetViewMatrix();
    float xOffset_clip = (2 * x_offset) / (float) m_screenWidth;
    float yOffset_clip = (2 * y_offset) / (float) m_screenHeight;
    glm::mat4 transform = glm::translate(glm::mat4(1.0), glm::vec3(xOffset_clip, yOffset_clip, 0));
    transform = glm::inverse(projection * view) * transform * projection * view;
    float x = transform[3][0];
    float y = transform[3][1];
    float z = transform[3][2];
    transform = glm::translate(glm::mat4(1.0f), glm::vec3(x, y, z));
    return transform;
}
