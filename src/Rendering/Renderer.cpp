#include "Rendering/Renderer.h"

#include "Camera/Camera.h"
#include "Core/Log.h"

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
    createSceneGeometry();
    m_initialized = true;

    Log::info("Renderer initialized.");
}

void Renderer::shutdown() {
    if (!m_initialized) {
        return;
    }

    destroySceneGeometry();
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

    m_shader.bind();

    const glm::mat4 view = camera.viewMatrix();
    const glm::mat4 projection = camera.projectionMatrix();
    const glm::mat4 model = glm::mat4(1.0f);

    const int viewLoc = glGetUniformLocation(m_shader.id(), "uView");
    const int projLoc = glGetUniformLocation(m_shader.id(), "uProjection");
    const int modelLoc = glGetUniformLocation(m_shader.id(), "uModel");
    const int colorLoc = glGetUniformLocation(m_shader.id(), "uColor");

    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, &view[0][0]);
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, &projection[0][0]);
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, &model[0][0]);
    glUniform3f(colorLoc, 0.35f, 0.55f, 0.95f);

    glBindVertexArray(m_vao);
    glDrawElements(GL_TRIANGLES, m_indexCount, GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);

    m_shader.unbind();
}

void Renderer::createSceneGeometry() {
    // Reference cube at the origin so camera movement is visible immediately.
    const float vertices[] = {
        // positions
        -0.5f, -0.5f, -0.5f,
         0.5f, -0.5f, -0.5f,
         0.5f,  0.5f, -0.5f,
        -0.5f,  0.5f, -0.5f,
        -0.5f, -0.5f,  0.5f,
         0.5f, -0.5f,  0.5f,
         0.5f,  0.5f,  0.5f,
        -0.5f,  0.5f,  0.5f,
    };

    const unsigned int indices[] = {
        0, 1, 2, 2, 3, 0,
        4, 5, 6, 6, 7, 4,
        0, 1, 5, 5, 4, 0,
        2, 3, 7, 7, 6, 2,
        0, 3, 7, 7, 4, 0,
        1, 2, 6, 6, 5, 1,
    };

    m_indexCount = static_cast<int>(sizeof(indices) / sizeof(indices[0]));

    glGenVertexArrays(1, &m_vao);
    glGenBuffers(1, &m_vbo);
    glGenBuffers(1, &m_ebo);

    glBindVertexArray(m_vao);

    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);
    glEnableVertexAttribArray(0);

    glBindVertexArray(0);
}

void Renderer::destroySceneGeometry() {
    if (m_ebo != 0) {
        glDeleteBuffers(1, &m_ebo);
        m_ebo = 0;
    }
    if (m_vbo != 0) {
        glDeleteBuffers(1, &m_vbo);
        m_vbo = 0;
    }
    if (m_vao != 0) {
        glDeleteVertexArrays(1, &m_vao);
        m_vao = 0;
    }
}

} // namespace bhs::rendering
