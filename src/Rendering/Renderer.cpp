#include "Rendering/Renderer.h"

#include "Camera/Camera.h"
#include "Core/Log.h"
#include "Physics/PhysicsWorld.h"
#include "Rendering/Primitives.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <cstdio> // [PHASE3-DEBUG] snprintf for hex-formatted glGetError() logging

namespace bhs::rendering {
    using bhs::core::Log;

    Renderer& Renderer::instance() {
        static Renderer renderer;
        return renderer;
    }

    void Renderer::initialize(float eventHorizonRadius, float photonSphereRadius) {
        if (m_initialized) {
            return;
        }

        m_shader.loadFromFiles("assets/shaders/basic.vert", "assets/shaders/basic.frag");

        m_cubeMesh.upload(createCubeData());
        m_sphereMesh.upload(createUVSphereData());
        m_useCube = false; // sphere is the default reference shape

        m_starShader.loadFromFiles("assets/shaders/star.vert", "assets/shaders/star.frag");
        m_starMesh.upload(createStarfieldData(), GL_POINTS);

        // Event horizon: reuses the same UV sphere generator and basic
        // shader as the debug reference shape, just sized to the physically
        // computed radius and drawn solid black. No new shader needed.
        m_horizonMesh.upload(createUVSphereData(eventHorizonRadius));

        // Photon sphere: same generator/shader again, sized to its own
        // physically computed radius. Drawn as a wireframe (see render())
        // rather than solid, so it stays visually distinct from the solid
        // black horizon.
        m_photonSphereMesh.upload(createUVSphereData(photonSphereRadius));

        // Milestone 5, Phase 1: fullscreen-triangle scaffold. Added last, after
        // all M1-M4 setup above is complete and unmodified.
        initLensingPass();

        // Milestone 5, Phase 2: bake the existing starfield into a cubemap.
        // Runs after m_starShader/m_starMesh above are fully set up, since
        // it reuses them directly. One-time only -- not called from
        // render(). Not wired into lensing.frag yet (Phase 3).
        bakeEnvironment();

        m_initialized = true;

        Log::info("Renderer initialized.");
    }

    void Renderer::initLensingPass() {
        m_lensingShader.loadFromFiles("assets/shaders/lensing.vert", "assets/shaders/lensing.frag");

        // No vertex data is logically needed: the triangle's positions come
        // entirely from gl_VertexID in lensing.vert. A VAO must still be
        // bound for the draw call to be valid in core profile.
        //
        // [PHASE3-DEBUG] Hardening fix: a VAO with *zero* enabled vertex
        // attribute arrays is spec-legal for a gl_VertexID-only draw, but a
        // number of real-world OpenGL drivers (some Intel iGPU and older
        // NVIDIA/AMD combos in particular) have had bugs where
        // glDrawArrays against such a VAO silently draws nothing at all --
        // no GL error, no crash, just zero fragments. Since "no visible
        // change" is exactly the reported symptom, this is cheap and safe
        // to rule out: bind a single dummy attribute (attribute 0) to a
        // throwaway 1-vertex buffer. It is never read by lensing.vert
        // (which only uses gl_VertexID), so this changes no rendering
        // behavior on drivers that were already working correctly -- it
        // only removes the failure mode on drivers that were not.
        glGenVertexArrays(1, &m_lensingVAO);
        glBindVertexArray(m_lensingVAO);

        glGenBuffers(1, &m_lensingDummyVBO);
        glBindBuffer(GL_ARRAY_BUFFER, m_lensingDummyVBO);
        const float dummy[3] = { 0.0f, 0.0f, 0.0f };
        glBufferData(GL_ARRAY_BUFFER, sizeof(dummy), dummy, GL_STATIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
        glEnableVertexAttribArray(0);

        glBindVertexArray(0);

        Log::info("[PHASE3-DEBUG] Lensing pass initialized. programId=" +
            std::to_string(m_lensingShader.id()) + " vao=" + std::to_string(m_lensingVAO));
    }

    void Renderer::setLensingEnabled(bool enabled) {
        // [PHASE3-DEBUG] Confirms checkpoints A/B: this fires exactly when
        // ImGuiLayer's checkbox callback runs, proving the ImGui -> Renderer
        // call actually happens and showing the value it passed.
        if (enabled && !m_lensingEnabled) {
            m_lensingJustEnabled = true; // re-arm the one-shot diagnostic log below
        }
        Log::info(std::string("[PHASE3-DEBUG] setLensingEnabled(") + (enabled ? "true" : "false") + ") called.");
        m_lensingEnabled = enabled;
    }

    void Renderer::bakeEnvironment() {
        // Reuses the existing starfield shader/mesh exactly as built above
        // in initialize() -- no second starfield algorithm, no duplicated
        // generation logic. See EnvironmentBaker for the per-face render.
        m_environmentBaker.bake(m_starShader, m_starMesh);
    }

    void Renderer::shutdown() {
        if (!m_initialized) {
            return;
        }

        m_cubeMesh.release();
        m_sphereMesh.release();
        m_starMesh.release();
        m_horizonMesh.release();
        m_photonSphereMesh.release();

        if (m_lensingVAO != 0) {
            glDeleteVertexArrays(1, &m_lensingVAO);
            m_lensingVAO = 0;
        }
        if (m_lensingDummyVBO != 0) {
            glDeleteBuffers(1, &m_lensingDummyVBO);
            m_lensingDummyVBO = 0;
        }

        m_environmentBaker.release();

        m_initialized = false;
    }

    void Renderer::render(const camera::Camera& camera) {
        int width = 0;
        int height = 0;
        glfwGetFramebufferSize(glfwGetCurrentContext(), &width, &height);
        if (width > 0 && height > 0) {
            glViewport(0, 0, width, height);
        }

        glClearColor(0.02f, 0.02f, 0.05f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Milestone 5: when enabled, the fullscreen lensing pass replaces the
        // scene below entirely for this frame. The existing M1-M4 path from
        // here down is completely unmodified and remains the default
        // (m_lensingEnabled starts false) — toggling this off at runtime
        // reproduces the exact pre-M5 build behavior.
        if (m_lensingEnabled) {
            // [PHASE3-DEBUG] Confirms checkpoint A: proves render() actually
            // takes this branch instead of falling through to M1-M4 below.
            static bool loggedBranchOnce = false;
            if (!loggedBranchOnce) {
                Log::info("[PHASE3-DEBUG] render(): m_lensingEnabled=true -> taking lensing branch, skipping M1-M4 path.");
                loggedBranchOnce = true;
            }
            renderLensingPass(camera);
            return;
        }

        const glm::mat4 fullView = camera.viewMatrix();
        const glm::mat4 projection = camera.projectionMatrix();

        // Starfield pass: infinitely distant background, drawn first. State
        // (depth mask, blending) is scoped locally to this pass and restored
        // immediately after, rather than made part of OpenGLContext's
        // one-time global setup.
        {
            m_starShader.bind();

            // Strip translation from the view matrix so stars react to camera
            // rotation but not camera position (standard skybox convention).
            const glm::mat4 viewNoTranslation = glm::mat4(glm::mat3(fullView));

            const int starViewLoc = glGetUniformLocation(m_starShader.id(), "uView");
            const int starProjLoc = glGetUniformLocation(m_starShader.id(), "uProjection");

            glUniformMatrix4fv(starViewLoc, 1, GL_FALSE, &viewNoTranslation[0][0]);
            glUniformMatrix4fv(starProjLoc, 1, GL_FALSE, &projection[0][0]);

            glDepthMask(GL_FALSE);
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

            m_starMesh.draw();

            glDisable(GL_BLEND);
            glDepthMask(GL_TRUE);

            m_starShader.unbind();
        }

        m_shader.bind();

        const glm::mat4& view = fullView;
        const glm::mat4 model = glm::mat4(1.0f);

        const int viewLoc = glGetUniformLocation(m_shader.id(), "uView");
        const int projLoc = glGetUniformLocation(m_shader.id(), "uProjection");
        const int modelLoc = glGetUniformLocation(m_shader.id(), "uModel");
        const int colorLoc = glGetUniformLocation(m_shader.id(), "uColor");

        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, &view[0][0]);
        glUniformMatrix4fv(projLoc, 1, GL_FALSE, &projection[0][0]);
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, &model[0][0]);
        glUniform3f(colorLoc, 0.35f, 0.55f, 0.95f);

        const Mesh& activeMesh = m_useCube ? m_cubeMesh : m_sphereMesh;
        activeMesh.draw();

        // Event horizon: same shader/model/view/projection already bound
        // above, just a different color and mesh. Depth testing against the
        // debug shape and starfield resolves correctly regardless of draw
        // order since all three are opaque geometry.
        glUniform3f(colorLoc, 0.0f, 0.0f, 0.0f);
        m_horizonMesh.draw();

        // Photon sphere: drawn as a wireframe so it stays visually distinct
        // from the solid black horizon. Polygon mode and line width are
        // scoped to just this draw call and restored immediately after,
        // matching the pattern already used for the starfield's blend/depth
        // state — this is drawcall-specific, not global context setup.
        glUniform3f(colorLoc, 1.0f, 0.6f, 0.1f);
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        glLineWidth(2.0f);

        m_photonSphereMesh.draw();

        glLineWidth(1.0f);
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

        m_shader.unbind();
    }

    void Renderer::renderLensingPass(const camera::Camera& camera) {
        // Milestone 5: reconstructs a world-space ray per pixel in
        // lensing.frag and integrates the Schwarzschild null-geodesic orbit
        // equation to find each ray's true deflected/captured fate before
        // sampling the Phase 2 environment cubemap. The inverse matrices
        // are computed here rather than in the shader since they are
        // per-frame-constant (identical for all three triangle
        // vertices/every fragment this draw), not per-pixel work.
        //
        // Camera::viewMatrix()/projectionMatrix()/position() are the same
        // accessors the existing M1-M4 path already uses (see render()
        // above) -- Camera itself is untouched. PhysicsWorld::mass()/
        // schwarzschildRadius() are likewise pre-existing read-only
        // accessors (see ImGuiLayer.cpp for the same access pattern) --
        // PhysicsWorld itself is untouched.
        const glm::mat4 invProjection = glm::inverse(camera.projectionMatrix());
        const glm::mat4 invView = glm::inverse(camera.viewMatrix());
        const glm::vec3 cameraPos = camera.position();
        const auto& physicsWorld = physics::PhysicsWorld::instance();
        const float mass = physicsWorld.mass();
        const float schwarzschildRadius = physicsWorld.schwarzschildRadius();

        m_lensingShader.bind();

        const int invProjLoc = glGetUniformLocation(m_lensingShader.id(), "uInvProjection");
        const int invViewLoc = glGetUniformLocation(m_lensingShader.id(), "uInvView");
        const int cameraPosLoc = glGetUniformLocation(m_lensingShader.id(), "uCameraPos");
        const int massLoc = glGetUniformLocation(m_lensingShader.id(), "uMass");
        const int schwarzschildRadiusLoc = glGetUniformLocation(m_lensingShader.id(), "uSchwarzschildRadius");
        const int environmentLoc = glGetUniformLocation(m_lensingShader.id(), "uEnvironment");
        const int debugModeLoc = glGetUniformLocation(m_lensingShader.id(), "uDebugMode");

        // [PHASE3-DEBUG] One-shot full diagnostic dump covering checkpoints
        // C-G, logged the first frame after the toggle switches on so it
        // doesn't spam every frame at 60fps.
        if (m_lensingJustEnabled) {
            m_lensingJustEnabled = false;
            const bool baked = m_environmentBaker.isBaked();
            const unsigned int envTex = m_environmentBaker.environmentMap();
            Log::info("[PHASE3-DEBUG] --- Lensing pass diagnostic dump ---");
            Log::info("[PHASE3-DEBUG] shader programId=" + std::to_string(m_lensingShader.id()) +
                " (0 would mean compile/link failed -- but that throws at startup, so nonzero here is expected)");
            Log::info(std::string("[PHASE3-DEBUG] uniform locations: uInvProjection=") + std::to_string(invProjLoc) +
                " uInvView=" + std::to_string(invViewLoc) +
                " uCameraPos=" + std::to_string(cameraPosLoc) +
                " uMass=" + std::to_string(massLoc) +
                " uSchwarzschildRadius=" + std::to_string(schwarzschildRadiusLoc) +
                " uEnvironment=" + std::to_string(environmentLoc) +
                " uDebugMode=" + std::to_string(debugModeLoc) +
                " (-1 means the uniform name doesn't exist in the linked program or was optimized out)");
            Log::info(std::string("[PHASE3-DEBUG] EnvironmentBaker::isBaked()=") + (baked ? "true" : "false") +
                " environmentMap() textureId=" + std::to_string(envTex) +
                " (0 or isBaked()==false means checkpoint D/E failed -- cubemap was never created/populated)");
            Log::info("[PHASE3-DEBUG] lensingVAO=" + std::to_string(m_lensingVAO) +
                " dummyVBO=" + std::to_string(m_lensingDummyVBO));
        }

        glUniformMatrix4fv(invProjLoc, 1, GL_FALSE, &invProjection[0][0]);
        glUniformMatrix4fv(invViewLoc, 1, GL_FALSE, &invView[0][0]);
        glUniform3f(cameraPosLoc, cameraPos.x, cameraPos.y, cameraPos.z);
        glUniform1f(massLoc, mass);
        glUniform1f(schwarzschildRadiusLoc, schwarzschildRadius);
        // 0=normal (full geodesic lensing), 1=flat diagnostic color,
        // 2=raw world-ray-direction visualization. See lensing.frag.
        glUniform1i(debugModeLoc, m_lensingDebugMode);

        // Explicit texture unit binding: texture unit 0 is used deliberately
        // (not left implicit) and the sampler uniform is pointed at it
        // explicitly, so this cannot silently collide with texture state
        // from elsewhere. The existing M1-M4 path binds no textures at all,
        // so there is nothing for this to conflict with regardless, but this
        // keeps the lensing pass self-contained if that changes later.
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, m_environmentBaker.environmentMap());
        glUniform1i(environmentLoc, 0);

        // The fullscreen triangle has no meaningful depth (a single pass,
        // nothing else drawn in this branch of render()) and there is no
        // depth attachment concern here since this pass renders straight to
        // the default framebuffer -- but depth state is still saved and
        // restored explicitly rather than assumed, matching the pattern
        // already used for the starfield's blend/depth-mask state and the
        // Phase 2 bake's depth-test save/restore.
        const GLboolean depthTestWasEnabled = glIsEnabled(GL_DEPTH_TEST);
        glDisable(GL_DEPTH_TEST);

        glBindVertexArray(m_lensingVAO);
        glDrawArrays(GL_TRIANGLES, 0, 3);

        // [PHASE3-DEBUG] Checkpoint B: confirms the draw call itself issued
        // cleanly. Logged only once (guarded by the same flag pattern as
        // the dump above would double-fire otherwise, so use a separate
        // static flag scoped to just this check).
        {
            static bool loggedDrawOnce = false;
            if (!loggedDrawOnce) {
                const GLenum err = glGetError();
                char hexBuf[16] = {};
                snprintf(hexBuf, sizeof(hexBuf), "0x%04X", err);
                Log::info(std::string("[PHASE3-DEBUG] renderLensingPass: glDrawArrays(GL_TRIANGLES, 0, 3) issued. glGetError()=") +
                    hexBuf + (err == GL_NO_ERROR ? " (GL_NO_ERROR)" : " (ERROR -- see glad/glcorearb.h for code meaning)"));
                loggedDrawOnce = true;
            }
        }

        glBindVertexArray(0);

        if (depthTestWasEnabled) {
            glEnable(GL_DEPTH_TEST);
        }

        glBindTexture(GL_TEXTURE_CUBE_MAP, 0);

        m_lensingShader.unbind();
    }

} // namespace bhs::rendering