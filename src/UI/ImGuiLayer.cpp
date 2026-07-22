#include "UI/ImGuiLayer.h"

#include "Camera/CameraController.h"
#include "Core/Log.h"
#include "Core/Timer.h"

#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>

#include <GLFW/glfw3.h>

namespace bhs::ui {
    using bhs::core::Log;

namespace {
    core::Timer s_uiTimer;
}

ImGuiLayer& ImGuiLayer::instance() {
    static ImGuiLayer layer;
    return layer;
}

void ImGuiLayer::initialize(GLFWwindow* window) {
    if (m_initialized) {
        return;
    }

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    ImGui::StyleColorsDark();

    if (!ImGui_ImplGlfw_InitForOpenGL(window, false)) {
        Log::fatal("Failed to initialize ImGui GLFW backend.");
    }

    if (!ImGui_ImplOpenGL3_Init("#version 330 core")) {
        Log::fatal("Failed to initialize ImGui OpenGL3 backend.");
    }

    m_window = window;
    m_initialized = true;
    Log::info("ImGui layer initialized.");
}

void ImGuiLayer::shutdown() {
    if (!m_initialized) {
        return;
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    m_window = nullptr;
    m_initialized = false;
}

void ImGuiLayer::beginFrame() {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

void ImGuiLayer::render() {
    const auto& camera = camera::CameraController::instance().camera();
    const ImGuiIO& io = ImGui::GetIO();

    ImGui::Begin("Debug");
    ImGui::Text("Black Hole Simulator - Foundation Build");
    ImGui::Separator();
    ImGui::Text("FPS: %.1f", io.Framerate);
    ImGui::Text("Frame time: %.3f ms", 1000.0f / io.Framerate);
    ImGui::Text("Uptime: %.1f s", s_uiTimer.elapsedSeconds());
    ImGui::Separator();
    ImGui::Text("Camera position: (%.2f, %.2f, %.2f)",
        camera.position().x, camera.position().y, camera.position().z);
    ImGui::Text("Camera yaw: %.1f  pitch: %.1f  fov: %.1f",
        camera.yaw(), camera.pitch(), camera.fov());
    ImGui::Separator();
    ImGui::Text("Controls");
    ImGui::BulletText("W/A/S/D - move");
    ImGui::BulletText("Q/E - down/up");
    ImGui::BulletText("Right mouse - look");
    ImGui::BulletText("Scroll - zoom (FOV)");
    ImGui::BulletText("Esc - quit");
    ImGui::End();
}

void ImGuiLayer::endFrame() {
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

} // namespace bhs::ui
