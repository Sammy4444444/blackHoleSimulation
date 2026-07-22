#pragma once

namespace bhs::core {

class Window;

class Application {
public:
    Application();
    ~Application();

    Application(const Application&) = delete;
    Application& operator=(const Application&) = delete;

    int run();

private:
    void initialize();
    void shutdown();
    void update(float deltaTime);
    void render();

    Window* m_window = nullptr;
    bool m_running = false;
};

} // namespace bhs::core
