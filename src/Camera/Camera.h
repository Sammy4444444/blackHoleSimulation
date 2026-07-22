#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace bhs::camera {

class Camera {
public:
    Camera();

    void setPerspective(float fovDegrees, float aspectRatio, float nearPlane, float farPlane);
    void setAspectRatio(float aspectRatio);

    void setPosition(const glm::vec3& position);
    void setRotation(float yawDegrees, float pitchDegrees);

    void moveLocal(const glm::vec3& offset);
    void rotate(float yawDelta, float pitchDelta);

    const glm::vec3& position() const { return m_position; }
    float yaw() const { return m_yaw; }
    float pitch() const { return m_pitch; }
    float fov() const { return m_fov; }
    float aspectRatio() const { return m_aspectRatio; }

    void setFov(float fovDegrees);

    glm::mat4 viewMatrix() const;
    glm::mat4 projectionMatrix() const;
    glm::vec3 forward() const;
    glm::vec3 right() const;
    glm::vec3 up() const;

private:
    void clampPitch();

    glm::vec3 m_position{0.0f, 0.0f, 5.0f};
    float m_yaw = -90.0f;
    float m_pitch = 0.0f;

    float m_fov = 60.0f;
    float m_aspectRatio = 16.0f / 9.0f;
    float m_nearPlane = 0.1f;
    float m_farPlane = 5000.0f;
};

} // namespace bhs::camera
