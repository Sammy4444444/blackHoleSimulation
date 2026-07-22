#include "Camera/CameraController.h"

#include <GLFW/glfw3.h>

#include <algorithm>

namespace bhs::camera {

CameraController& CameraController::instance() {
    static CameraController controller;
    return controller;
}

void CameraController::initialize(GLFWwindow* window) {
    m_window = window;

    glfwSetCursorPosCallback(window, mouseCallback);
    glfwSetMouseButtonCallback(window, mouseButtonCallback);
    glfwSetScrollCallback(window, scrollCallback);

    updateAspectRatio();
}

void CameraController::update(float deltaTime) {
    processKeyboard(deltaTime);
    updateAspectRatio();
}

void CameraController::mouseCallback(GLFWwindow* window, double xpos, double ypos) {
    CameraController& self = instance();
    if (!self.m_rotating || glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) != GLFW_PRESS) {
        return;
    }

    if (self.m_firstMouse) {
        self.m_lastMouseX = xpos;
        self.m_lastMouseY = ypos;
        self.m_firstMouse = false;
        return;
    }

    const double xoffset = xpos - self.m_lastMouseX;
    const double yoffset = self.m_lastMouseY - ypos;
    self.m_lastMouseX = xpos;
    self.m_lastMouseY = ypos;

    self.m_camera.rotate(
        static_cast<float>(xoffset) * self.m_mouseSensitivity,
        static_cast<float>(yoffset) * self.m_mouseSensitivity);
}

void CameraController::mouseButtonCallback(GLFWwindow* window, int button, int action, int /*mods*/) {
    CameraController& self = instance();

    if (button == GLFW_MOUSE_BUTTON_RIGHT) {
        if (action == GLFW_PRESS) {
            self.m_rotating = true;
            self.m_firstMouse = true;
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        } else if (action == GLFW_RELEASE) {
            self.m_rotating = false;
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        }
    }
}

void CameraController::scrollCallback(GLFWwindow* /*window*/, double /*xoffset*/, double yoffset) {
    CameraController& self = instance();

    float fov = self.m_camera.fov() - static_cast<float>(yoffset) * self.m_scrollSensitivity;
    fov = std::clamp(fov, 20.0f, 90.0f);
    self.m_camera.setFov(fov);
}

void CameraController::processKeyboard(float deltaTime) {
    if (!m_window) {
        return;
    }

    if (glfwGetKey(m_window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        glfwSetWindowShouldClose(m_window, GLFW_TRUE);
    }

    const float speed = m_moveSpeed * deltaTime;
    glm::vec3 movement(0.0f);

    if (glfwGetKey(m_window, GLFW_KEY_W) == GLFW_PRESS) movement.z += speed;
    if (glfwGetKey(m_window, GLFW_KEY_S) == GLFW_PRESS) movement.z -= speed;
    if (glfwGetKey(m_window, GLFW_KEY_A) == GLFW_PRESS) movement.x -= speed;
    if (glfwGetKey(m_window, GLFW_KEY_D) == GLFW_PRESS) movement.x += speed;
    if (glfwGetKey(m_window, GLFW_KEY_E) == GLFW_PRESS) movement.y += speed;
    if (glfwGetKey(m_window, GLFW_KEY_Q) == GLFW_PRESS) movement.y -= speed;

    if (movement != glm::vec3(0.0f)) {
        m_camera.moveLocal(movement);
    }
}

void CameraController::updateAspectRatio() {
    int width = 0;
    int height = 0;
    glfwGetFramebufferSize(m_window, &width, &height);

    if (width <= 0 || height <= 0) {
        return;
    }

    m_camera.setAspectRatio(static_cast<float>(width) / static_cast<float>(height));
}

} // namespace bhs::camera
