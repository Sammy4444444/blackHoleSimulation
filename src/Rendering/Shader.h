#pragma once

#include <string>

namespace bhs::rendering {

class Shader {
public:
    Shader() = default;
    ~Shader();

    Shader(const Shader&) = delete;
    Shader& operator=(const Shader&) = delete;
    Shader(Shader&& other) noexcept;
    Shader& operator=(Shader&& other) noexcept;

    void loadFromFiles(const std::string& vertexPath, const std::string& fragmentPath);
    void bind() const;
    void unbind() const;

    unsigned int id() const { return m_programId; }

private:
    unsigned int compileStage(unsigned int type, const std::string& source, const std::string& path);
    void linkProgram(unsigned int vertexShader, unsigned int fragmentShader);

    unsigned int m_programId = 0;
};

} // namespace bhs::rendering
