#include "UI/ImGuiLayer.h"

#include "Camera/CameraController.h"
#include "Core/Log.h"
#include "Core/Timer.h"
#include "Physics/PhysicsWorld.h"
#include "Rendering/Renderer.h"

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

    ImGui::Begin("Debug", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
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
    ImGui::Text("Black Hole (read-only)");
    const auto& physicsWorld = physics::PhysicsWorld::instance();
    const float schwarzschildRadius = physicsWorld.schwarzschildRadius();
    const float photonSphereRadius = physicsWorld.photonSphereRadius();
    ImGui::Text("Mass: %.2f", physicsWorld.mass());
    ImGui::Text("Event Horizon Rs: %.2f", schwarzschildRadius);
    ImGui::Text("Photon Sphere Rp: %.2f", photonSphereRadius);
    ImGui::Text("Rp / Rs: %.2f", photonSphereRadius / schwarzschildRadius);
    ImGui::Separator();
    ImGui::Text("Milestone 5: Gravitational Lensing");
    {
        auto& renderer = rendering::Renderer::instance();
        bool lensingEnabled = renderer.lensingEnabled();
        if (ImGui::Checkbox("Enable Lensing Pass (Schwarzschild geodesic lensing)", &lensingEnabled)) {
            renderer.setLensingEnabled(lensingEnabled);
        }

        // Diagnostic selector, still useful for isolating pipeline stages
        // from lensing-physics correctness: prove the branch executes
        // (flat color) / ray reconstruction is sane (raw direction) /
        // trust the real lensed cubemap sample (normal).
        if (lensingEnabled) {
            static const char* kModes[] = {
                "0: Normal (Schwarzschild geodesic lensing)",
                "1: Diagnostic - flat magenta",
                "2: Diagnostic - raw ray direction as RGB"
            };
            int debugMode = renderer.lensingDebugMode();
            if (ImGui::Combo("Lensing debug mode", &debugMode, kModes, 3)) {
                renderer.setLensingDebugMode(debugMode);
            }
        }
    }
    ImGui::Separator();
    ImGui::Text("Milestone 6: Accretion Disk");
    {
        auto& renderer = rendering::Renderer::instance();
        bool diskEnabled = renderer.diskEnabled();
        if (ImGui::Checkbox("Enable Accretion Disk", &diskEnabled)) {
            renderer.setDiskEnabled(diskEnabled);
        }
        if (!renderer.lensingEnabled()) {
            ImGui::TextDisabled("(has no effect until the lensing pass above is enabled)");
        }

        if (diskEnabled) {
            // Clamp bounds: inner radius can never be dragged below the
            // event horizon (the shader-side gate already prevents a
            // sub-Rs disk from being visible, but keeping the slider's own
            // range honest avoids a UI control that silently does nothing
            // for part of its range). Outer radius is kept above the
            // current inner radius for the same reason.
            // schwarzschildRadius here is the same variable already computed
            // above for the "Black Hole (read-only)" section.
            float innerRadius = renderer.diskInnerRadius();
            float outerRadius = renderer.diskOuterRadius();
            float referenceTemperature = renderer.diskReferenceTemperature();
            float brightness = renderer.diskBrightness();

            if (ImGui::SliderFloat("Inner Radius", &innerRadius, schwarzschildRadius, outerRadius - 1e-3f)) {
                renderer.setDiskInnerRadius(innerRadius);
            }
            if (ImGui::SliderFloat("Outer Radius", &outerRadius, innerRadius + 1e-3f, 100.0f * schwarzschildRadius)) {
                renderer.setDiskOuterRadius(outerRadius);
            }
            if (ImGui::SliderFloat("Reference Temperature", &referenceTemperature, 0.05f, 5.0f)) {
                renderer.setDiskReferenceTemperature(referenceTemperature);
            }
            if (ImGui::SliderFloat("Brightness", &brightness, 0.0f, 50.0f)) {
                renderer.setDiskBrightness(brightness);
            }
            ImGui::TextDisabled("Inner radius defaults to the ISCO (3*Rs); temperature/brightness are unitless.");
        }
    }
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
