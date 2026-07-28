#include "Camera/CameraController.h"

#include <GLFW/glfw3.h>

#include <imgui.h>
#include <backends/imgui_impl_glfw.h>

#include <algorithm>

namespace bhs::camera {

CameraController& CameraController::instance() {
    static CameraController controller;
    return controller;
}

void CameraController::initialize(GLFWwindow* window) {
    m_window = window;

    // --- Input-routing fix ------------------------------------------------
    // ImGuiLayer::initialize() calls ImGui_ImplGlfw_InitForOpenGL(window,
    // false) -- install_callbacks=false -- specifically so this class can
    // decide, per event, whether a click/scroll/key belongs to the camera
    // or to a UI widget (this is the "ImGui --> Cam" edge already drawn in
    // docs/ARCHITECTURE.md's layer diagram). But that also means ImGui
    // receives NO raw GLFW events at all unless something forwards them,
    // and until this fix nothing did: these three glfwSet*Callback calls
    // below are the ONLY callbacks ever installed on the window (they fully
    // replace, not chain with, anything ImGui might otherwise have
    // registered), and none of them told ImGui a click/scroll happened. The
    // practical symptom was every ImGui widget being permanently
    // unclickable -- io.MouseDown[] never became true, since only a
    // forwarded callback event can set it. mouseCallback/
    // mouseButtonCallback/scrollCallback below now forward to ImGui first,
    // then apply ImGui::GetIO().WantCaptureMouse to decide whether the
    // camera should also react. Key/char events had the same gap (also
    // never forwarded, so the ImGuiConfigFlags_NavEnableKeyboard flag
    // ImGuiLayer sets was silently inert, e.g. Ctrl+click-to-type-a-value
    // on a slider could never work) -- fixed the same way, by forwarding
    // directly to ImGui's own callback (no camera-specific keyboard
    // callback exists, since WASD movement polls glfwGetKey directly in
    // processKeyboard() rather than using the GLFW callback system, so
    // there is no existing logic here to preserve).
    glfwSetKeyCallback(window, ImGui_ImplGlfw_KeyCallback);
    glfwSetCharCallback(window, ImGui_ImplGlfw_CharCallback);

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
    ImGui_ImplGlfw_MouseButtonCallback(window, button, action, mods);

    CameraController& self = instance();

    if (button == GLFW_MOUSE_BUTTON_RIGHT) {
        if (action == GLFW_PRESS) {
            // Don't start an orbit drag on a click that landed on an ImGui
            // window/widget -- otherwise right-clicking the debug panel
            // would simultaneously grab and hide the cursor out from
            // under it.
            if (ImGui::GetIO().WantCaptureMouse) {
                return;
            }
            self.m_rotating = true;
            self.m_firstMouse = true;
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        } else if (action == GLFW_RELEASE) {
            // Always process release regardless of WantCaptureMouse, so a
            // rotation already in progress can't get stuck "on": the
            // cursor is disabled/hidden for the duration of a rotation, so
            // ImGui cannot meaningfully know it's "over" a window at
            // release time anyway.
            self.m_rotating = false;
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        }
    }
}

void CameraController::scrollCallback(GLFWwindow* window, double xoffset, double yoffset) {
    ImGui_ImplGlfw_ScrollCallback(window, xoffset, yoffset);

    if (ImGui::GetIO().WantCaptureMouse) {
        return; // Scrolling over the debug panel scrolls the panel, not the camera FOV.
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

    // Same routing gap as the mouse callbacks above, but narrower: gate
    // only on WantTextInput (true while an ImGui text field is actively
    // being edited, e.g. after Ctrl+click on a slider to type an exact
    // value), NOT the broader WantCaptureKeyboard. WantCaptureKeyboard is
    // persistently true whenever ImGuiConfigFlags_NavEnableKeyboard is set
    // (already the case here, from ImGuiLayer::initialize()) and any ImGui
    // window has nav focus -- which, with a single always-visible Debug
    // window, is essentially all the time. Gating on it blocked WASD
    // entirely rather than only while actually typing into a field.
    if (ImGui::GetIO().WantTextInput) {
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
