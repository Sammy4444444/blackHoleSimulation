#include "Core/Application.h"
#include "Core/Window.h"
#include "Core/Log.h"
#include "Core/Timer.h"
#include "Rendering/OpenGLContext.h"
#include "Rendering/Renderer.h"
#include "Camera/Camera.h"
#include "Camera/CameraController.h"
#include "UI/ImGuiLayer.h"
#include "Physics/PhysicsWorld.h"

#include <memory>

namespace bhs::core {

Application::Application() = default;

Application::~Application() {
    shutdown();
}

int Application::run() {
    try {
        initialize();

        Timer timer;
        while (m_running && !m_window->shouldClose()) {
            const float deltaTime = timer.tick();
            m_window->pollEvents();

            update(deltaTime);
            render();

            m_window->swapBuffers();
        }

        shutdown();
        return 0;
    } catch (const std::exception& ex) {
        Log::error(std::string("Unhandled exception: ") + ex.what());
        shutdown();
        return 1;
    }
}

void Application::initialize() {
    Log::info("Initializing Black Hole Simulator...");

    m_window = new Window(1600, 900, "Black Hole Simulator");
    rendering::OpenGLContext::initialize(m_window->handle());

    physics::PhysicsWorld::instance().initialize();

    rendering::Renderer::instance().initialize(physics::PhysicsWorld::instance().schwarzschildRadius());
    ui::ImGuiLayer::instance().initialize(m_window->handle());

    camera::CameraController::instance().initialize(m_window->handle());

    m_running = true;
    Log::info("Initialization complete.");
}

void Application::shutdown() {
    if (m_window == nullptr) {
        return;
    }

    Log::info("Shutting down...");

    ui::ImGuiLayer::instance().shutdown();
    rendering::Renderer::instance().shutdown();
    physics::PhysicsWorld::instance().shutdown();

    delete m_window;
    m_window = nullptr;
    m_running = false;
}

void Application::update(float deltaTime) {
    camera::CameraController::instance().update(deltaTime);
    physics::PhysicsWorld::instance().update(deltaTime);
    ui::ImGuiLayer::instance().beginFrame();
}

void Application::render() {
    const auto& camera = camera::CameraController::instance().camera();
    rendering::Renderer::instance().render(camera);
    ui::ImGuiLayer::instance().render();
    ui::ImGuiLayer::instance().endFrame();
}

} // namespace bhs::core
