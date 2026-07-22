#include "Camera/Camera.h"

#include <algorithm>
#include <cmath>

namespace bhs::camera {

Camera::Camera() {
    setPerspective(m_fov, m_aspectRatio, m_nearPlane, m_farPlane);
}

void Camera::setPerspective(float fovDegrees, float aspectRatio, float nearPlane, float farPlane) {
    m_fov = fovDegrees;
    m_aspectRatio = aspectRatio;
    m_nearPlane = nearPlane;
    m_farPlane = farPlane;
}

void Camera::setFov(float fovDegrees) {
    m_fov = fovDegrees;
}

void Camera::setAspectRatio(float aspectRatio) {
    m_aspectRatio = aspectRatio;
}

void Camera::setPosition(const glm::vec3& position) {
    m_position = position;
}

void Camera::setRotation(float yawDegrees, float pitchDegrees) {
    m_yaw = yawDegrees;
    m_pitch = pitchDegrees;
    clampPitch();
}

void Camera::moveLocal(const glm::vec3& offset) {
    m_position += right() * offset.x;
    m_position += up() * offset.y;
    m_position += forward() * offset.z;
}

void Camera::rotate(float yawDelta, float pitchDelta) {
    m_yaw += yawDelta;
    m_pitch += pitchDelta;
    clampPitch();
}

glm::mat4 Camera::viewMatrix() const {
    return glm::lookAt(m_position, m_position + forward(), glm::vec3(0.0f, 1.0f, 0.0f));
}

glm::mat4 Camera::projectionMatrix() const {
    return glm::perspective(glm::radians(m_fov), m_aspectRatio, m_nearPlane, m_farPlane);
}

glm::vec3 Camera::forward() const {
    const float yawRad = glm::radians(m_yaw);
    const float pitchRad = glm::radians(m_pitch);

    glm::vec3 direction;
    direction.x = std::cos(yawRad) * std::cos(pitchRad);
    direction.y = std::sin(pitchRad);
    direction.z = std::sin(yawRad) * std::cos(pitchRad);
    return glm::normalize(direction);
}

glm::vec3 Camera::right() const {
    return glm::normalize(glm::cross(forward(), glm::vec3(0.0f, 1.0f, 0.0f)));
}

glm::vec3 Camera::up() const {
    return glm::normalize(glm::cross(right(), forward()));
}

void Camera::clampPitch() {
    m_pitch = std::clamp(m_pitch, -89.0f, 89.0f);
}

} // namespace bhs::camera
