#pragma once

#include "Rendering/Mesh.h"
#include "Rendering/Shader.h"

namespace bhs::camera {
    class Camera;
}

namespace bhs::rendering {

    class Renderer {
    public:
        static Renderer& instance();

        void initialize(float eventHorizonRadius);
        void shutdown();
        void render(const camera::Camera& camera);

        // Debug aid: switches the rendered reference shape between the cube and
        // the UV sphere. Defaults to the sphere once both meshes exist.
        void setDebugCube(bool useCube) { m_useCube = useCube; }

    private:
        Renderer() = default;

        Shader m_shader;
        Mesh m_cubeMesh;
        Mesh m_sphereMesh;
        Shader m_starShader;
        Mesh m_starMesh;
        Mesh m_horizonMesh;
        bool m_useCube = false;
        bool m_initialized = false;
    };

} // namespace bhs::rendering