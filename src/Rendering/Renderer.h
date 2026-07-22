#pragma once

#include "Rendering/Shader.h"

#include <vector>

namespace bhs::camera {
class Camera;
}

namespace bhs::rendering {

class Renderer {
public:
    static Renderer& instance();

    void initialize();
    void shutdown();
    void render(const camera::Camera& camera);

private:
    Renderer() = default;

    void createSceneGeometry();
    void destroySceneGeometry();

    Shader m_shader;
    unsigned int m_vao = 0;
    unsigned int m_vbo = 0;
    unsigned int m_ebo = 0;
    int m_indexCount = 0;
    bool m_initialized = false;
};

} // namespace bhs::rendering
