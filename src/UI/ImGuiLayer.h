#pragma once

struct GLFWwindow;

namespace bhs::ui {

class ImGuiLayer {
public:
    static ImGuiLayer& instance();

    void initialize(GLFWwindow* window);
    void shutdown();
    void beginFrame();
    void render();
    void endFrame();

private:
    ImGuiLayer() = default;

    GLFWwindow* m_window = nullptr;
    bool m_initialized = false;
};

} // namespace bhs::ui
