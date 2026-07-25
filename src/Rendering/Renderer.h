#pragma once

#include "Rendering/Mesh.h"
#include "Rendering/Shader.h"
#include "Rendering/EnvironmentBaker.h"

namespace bhs::camera {
    class Camera;
}

namespace bhs::rendering {

    class Renderer {
    public:
        static Renderer& instance();

        void initialize(float eventHorizonRadius, float photonSphereRadius);
        void shutdown();
        void render(const camera::Camera& camera);

        // Debug aid: switches the rendered reference shape between the cube and
        // the UV sphere. Defaults to the sphere once both meshes exist.
        void setDebugCube(bool useCube) { m_useCube = useCube; }

        // Milestone 5: toggles between the existing M1-M4 render path
        // (unchanged) and the fullscreen lensing pass, which now performs
        // full Schwarzschild geodesic gravitational lensing (see
        // lensing.frag). Off by default so the application's visible
        // behavior is identical to the pre-M5 build unless explicitly
        // switched on (from ImGui).
        void setLensingEnabled(bool enabled);
        bool lensingEnabled() const { return m_lensingEnabled; }

        // Diagnostic modes for the lensing pass, still useful for isolating
        // pipeline stages from lensing-physics correctness: 0 = normal
        // (real geodesic lensing output), 1 = flat solid diagnostic color,
        // 2 = raw reconstructed world-ray direction visualized as RGB.
        void setLensingDebugMode(int mode) { m_lensingDebugMode = mode; }
        int lensingDebugMode() const { return m_lensingDebugMode; }

        // Milestone 5, Phase 2: the baked starfield environment cubemap,
        // produced once at startup (see initialize()) and sampled by the
        // lensing pass with each ray's final deflected/asymptotic
        // direction.
        unsigned int environmentMap() const { return m_environmentBaker.environmentMap(); }

    private:
        Renderer() = default;

        // Fullscreen-triangle pass (Phase 1 scaffold). Draws with
        // glDrawArrays(GL_TRIANGLES, 0, 3) against this empty VAO; positions
        // are generated in lensing.vert from gl_VertexID, so no VBO/EBO is
        // needed and Mesh is intentionally not reused here (Mesh exists to
        // upload CPU-side vertex data, which does not apply to this draw).
        void initLensingPass();

        // Milestone 5, Phase 3: needs the camera's inverse projection/view
        // matrices to reconstruct a world-space ray per pixel in
        // lensing.frag, so the camera is now threaded through here (it
        // wasn't needed for the Phase 1 flat-color placeholder). Camera
        // itself is untouched -- only its existing viewMatrix()/
        // projectionMatrix() accessors are used.
        void renderLensingPass(const camera::Camera& camera);

        Shader m_shader;
        Mesh m_cubeMesh;
        Mesh m_sphereMesh;
        Shader m_starShader;
        Mesh m_starMesh;
        Mesh m_horizonMesh;
        Mesh m_photonSphereMesh;
        bool m_useCube = false;
        bool m_initialized = false;

        // Milestone 5 additions (Phase 1 scope only):
        Shader m_lensingShader;
        unsigned int m_lensingVAO = 0;
        unsigned int m_lensingDummyVBO = 0; // [PHASE3-DEBUG] see initLensingPass()
        bool m_lensingEnabled = false;
        int m_lensingDebugMode = 0; // [PHASE3-DEBUG] 0=normal, 1=flat color, 2=raw ray dir
        bool m_lensingJustEnabled = false; // [PHASE3-DEBUG] one-shot diagnostic log flag

        // Milestone 5, Phase 2: one-time starfield cubemap bake.
        void bakeEnvironment();
        EnvironmentBaker m_environmentBaker;
    };

} // namespace bhs::rendering