#pragma once
#include "Camera/Camera.h"

struct GLFWwindow;

namespace bhs::camera {

;

class CameraController {
public:
    static CameraController& instance();

    void initialize(GLFWwindow* window);
    void update(float deltaTime);

    Camera& camera() { return m_camera; }
    const Camera& camera() const { return m_camera; }

private:
    CameraController() = default;

    static void mouseCallback(GLFWwindow* window, double xpos, double ypos);
    static void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
    static void scrollCallback(GLFWwindow* window, double xoffset, double yoffset);

    void processKeyboard(float deltaTime);
    void updateAspectRatio();

    Camera m_camera;
    GLFWwindow* m_window = nullptr;

    bool m_firstMouse = true;
    double m_lastMouseX = 0.0;
    double m_lastMouseY = 0.0;
    bool m_rotating = false;

    float m_moveSpeed = 8.0f;
    float m_mouseSensitivity = 0.12f;
    float m_scrollSensitivity = 2.0f;
};

} // namespace bhs::camera
