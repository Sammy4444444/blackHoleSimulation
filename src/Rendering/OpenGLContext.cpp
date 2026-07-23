#include "Rendering/OpenGLContext.h"

#include "Core/Log.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

namespace bhs::rendering {

    using bhs::core::Log;

    void OpenGLContext::initialize(GLFWwindow* window) {
        if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress))) {
            Log::fatal("Failed to initialize GLAD OpenGL loader.");
        }

        logInfo();

        glEnable(GL_DEPTH_TEST);
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);
        glFrontFace(GL_CCW);
        glEnable(GL_PROGRAM_POINT_SIZE);
    }

    void OpenGLContext::logInfo() {
        const char* vendor = reinterpret_cast<const char*>(glGetString(GL_VENDOR));
        const char* renderer = reinterpret_cast<const char*>(glGetString(GL_RENDERER));
        const char* version = reinterpret_cast<const char*>(glGetString(GL_VERSION));

        if (!vendor || !renderer || !version) {
            Log::warn("Unable to query OpenGL strings.");
            return;
        }

        Log::info(std::string("OpenGL vendor:   ") + vendor);
        Log::info(std::string("OpenGL renderer: ") + renderer);
        Log::info(std::string("OpenGL version:  ") + version);
    }

} // namespace bhs::rendering