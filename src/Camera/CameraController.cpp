#include "Camera/CameraController.h"

#include <imgui.h>
#include <backends/imgui_impl_glfw.h>

#include <GLFW/glfw3.h>

#include <algorithm>

namespace bhs::camera {

CameraController& CameraController::instance() {
    static CameraController controller;
    return controller;
}

void CameraController::initialize(GLFWwindow* window) {
    m_window = window;

    // ImGuiLayer::initialize() calls ImGui_ImplGlfw_InitForOpenGL(window, false),
    // i.e. it deliberately does NOT let the ImGui GLFW backend install its own
    // raw GLFW callbacks, because CameraController (initialized afterwards)
    // needs to own cursor/mouse-button/scroll input for camera control.
    // GLFW only allows one callback per event type, so whichever callback is
    // installed last wins; without manual forwarding here, ImGui never
    // receives mouse position/button/scroll events at all and every ImGui
    // widget (including the lensing checkbox) is unclickable.
    // We therefore forward every raw event to the ImGui GLFW backend
    // ourselves first, then apply camera behavior only when ImGui itself
    // doesn't want to consume the input (io.WantCaptureMouse).
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
    // Forward first so ImGui's io.MousePos always reflects reality, even
    // while the camera is also consuming this event.
    ImGui_ImplGlfw_CursorPosCallback(window, xpos, ypos);

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

void CameraController::mouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
    // Forward first: ImGui needs every button press/release, including
    // clicks on the checkbox, or io.MouseDown[] never updates and
    // ImGui::Checkbox() can never register a click.
    ImGui_ImplGlfw_MouseButtonCallback(window, button, action, mods);

    CameraController& self = instance();

    if (button == GLFW_MOUSE_BUTTON_RIGHT) {
        if (action == GLFW_PRESS) {
            // Only start camera-look if ImGui isn't the one handling this
            // click (e.g. the cursor is over the Debug panel). This keeps
            // right-click-to-look from hijacking clicks meant for ImGui,
            // without touching how rotation itself works.
            if (ImGui::GetIO().WantCaptureMouse) {
                return;
            }
            self.m_rotating = true;
            self.m_firstMouse = true;
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        } else if (action == GLFW_RELEASE) {
            self.m_rotating = false;
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        }
    }
}

void CameraController::scrollCallback(GLFWwindow* window, double xoffset, double yoffset) {
    // Forward first so ImGui panels can be scrolled and so io.MouseWheel
    // reflects this event.
    ImGui_ImplGlfw_ScrollCallback(window, xoffset, yoffset);

    if (ImGui::GetIO().WantCaptureMouse) {
        return;
    }

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
