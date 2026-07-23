#include "Rendering/Renderer.h"

#include "Camera/Camera.h"
#include "Core/Log.h"
#include "Rendering/Primitives.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace bhs::rendering {
    using bhs::core::Log;

    Renderer& Renderer::instance() {
        static Renderer renderer;
        return renderer;
    }

    void Renderer::initialize() {
        if (m_initialized) {
            return;
        }

        m_shader.loadFromFiles("assets/shaders/basic.vert", "assets/shaders/basic.frag");

        m_cubeMesh.upload(createCubeData());
        m_sphereMesh.upload(createUVSphereData());
        m_useCube = false; // sphere is the default reference shape

        m_starShader.loadFromFiles("assets/shaders/star.vert", "assets/shaders/star.frag");
        m_starMesh.upload(createStarfieldData(), GL_POINTS);

        m_initialized = true;

        Log::info("Renderer initialized.");
    }

    void Renderer::shutdown() {
        if (!m_initialized) {
            return;
        }

        m_cubeMesh.release();
        m_sphereMesh.release();
        m_starMesh.release();
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

        m_shader.unbind();
    }

} // namespace bhs::rendering