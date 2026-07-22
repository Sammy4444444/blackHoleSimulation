#include "Rendering/Shader.h"

#include "Assets/AssetManager.h"
#include "Core/Log.h"

#include <glad/glad.h>

namespace bhs::rendering {
    using bhs::core::Log;

Shader::~Shader() {
    if (m_programId != 0) {
        glDeleteProgram(m_programId);
        m_programId = 0;
    }
}

Shader::Shader(Shader&& other) noexcept
    : m_programId(other.m_programId) {
    other.m_programId = 0;
}

Shader& Shader::operator=(Shader&& other) noexcept {
    if (this != &other) {
        if (m_programId != 0) {
            glDeleteProgram(m_programId);
        }
        m_programId = other.m_programId;
        other.m_programId = 0;
    }
    return *this;
}

void Shader::loadFromFiles(const std::string& vertexPath, const std::string& fragmentPath) {
    const std::string vertexSource = assets::AssetManager::readTextFile(vertexPath);
    const std::string fragmentSource = assets::AssetManager::readTextFile(fragmentPath);

    const unsigned int vertexShader = compileStage(GL_VERTEX_SHADER, vertexSource, vertexPath);
    const unsigned int fragmentShader = compileStage(GL_FRAGMENT_SHADER, fragmentSource, fragmentPath);

    linkProgram(vertexShader, fragmentShader);

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
}

void Shader::bind() const {
    glUseProgram(m_programId);
}

void Shader::unbind() const {
    glUseProgram(0);
}

unsigned int Shader::compileStage(unsigned int type, const std::string& source, const std::string& path) {
    const unsigned int shader = glCreateShader(type);
    const char* src = source.c_str();
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);

    int success = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[1024] = {};
        glGetShaderInfoLog(shader, sizeof(infoLog), nullptr, infoLog);
        glDeleteShader(shader);
        Log::fatal("Shader compilation failed (" + path + "): " + infoLog);
    }

    return shader;
}

void Shader::linkProgram(unsigned int vertexShader, unsigned int fragmentShader) {
    if (m_programId != 0) {
        glDeleteProgram(m_programId);
    }

    m_programId = glCreateProgram();
    glAttachShader(m_programId, vertexShader);
    glAttachShader(m_programId, fragmentShader);
    glLinkProgram(m_programId);

    int success = 0;
    glGetProgramiv(m_programId, GL_LINK_STATUS, &success);
    if (!success) {
        char infoLog[1024] = {};
        glGetProgramInfoLog(m_programId, sizeof(infoLog), nullptr, infoLog);
        glDeleteProgram(m_programId);
        m_programId = 0;
        Log::fatal(std::string("Shader program linking failed: ") + infoLog);
    }
}

} // namespace bhs::rendering
