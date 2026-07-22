#pragma once

struct GLFWwindow;

namespace bhs::rendering {

class OpenGLContext {
public:
    static void initialize(GLFWwindow* window);
    static void logInfo();
};

} // namespace bhs::rendering
