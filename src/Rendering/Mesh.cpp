#include "Rendering/Mesh.h"

#include <glad/glad.h>

#include <utility>

namespace bhs::rendering {

    Mesh::~Mesh() {
        release();
    }

    Mesh::Mesh(Mesh&& other) noexcept
        : m_vao(other.m_vao)
        , m_vbo(other.m_vbo)
        , m_ebo(other.m_ebo)
        , m_indexCount(other.m_indexCount)
        , m_vertexCount(other.m_vertexCount)
        , m_primitiveType(other.m_primitiveType) {
        other.m_vao = 0;
        other.m_vbo = 0;
        other.m_ebo = 0;
        other.m_indexCount = 0;
        other.m_vertexCount = 0;
    }

    Mesh& Mesh::operator=(Mesh&& other) noexcept {
        if (this != &other) {
            release();

            m_vao = other.m_vao;
            m_vbo = other.m_vbo;
            m_ebo = other.m_ebo;
            m_indexCount = other.m_indexCount;
            m_vertexCount = other.m_vertexCount;
            m_primitiveType = other.m_primitiveType;

            other.m_vao = 0;
            other.m_vbo = 0;
            other.m_ebo = 0;
            other.m_indexCount = 0;
            other.m_vertexCount = 0;
        }
        return *this;
    }

    void Mesh::upload(const MeshData& data, unsigned int primitiveType) {
        release();

        m_primitiveType = primitiveType;
        m_indexCount = static_cast<int>(data.indices.size());
        m_vertexCount = static_cast<int>(data.vertices.size() / 3);

        glGenVertexArrays(1, &m_vao);
        glGenBuffers(1, &m_vbo);

        glBindVertexArray(m_vao);

        glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
        glBufferData(GL_ARRAY_BUFFER, data.vertices.size() * sizeof(float),
            data.vertices.data(), GL_STATIC_DRAW);

        // Non-indexed geometry (e.g. a point cloud with no shared-vertex
        // topology) has no valid index buffer to build; skip the EBO entirely
        // rather than manufacturing a throwaway identity one.
        if (!data.indices.empty()) {
            glGenBuffers(1, &m_ebo);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ebo);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, data.indices.size() * sizeof(unsigned int),
                data.indices.data(), GL_STATIC_DRAW);
        }

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);
        glEnableVertexAttribArray(0);

        glBindVertexArray(0);
    }

    void Mesh::release() {
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
        m_indexCount = 0;
        m_vertexCount = 0;
    }

    void Mesh::bind() const {
        glBindVertexArray(m_vao);
    }

    void Mesh::unbind() const {
        glBindVertexArray(0);
    }

    void Mesh::draw() const {
        bind();
        if (m_ebo != 0) {
            glDrawElements(m_primitiveType, m_indexCount, GL_UNSIGNED_INT, nullptr);
        } else {
            glDrawArrays(m_primitiveType, 0, m_vertexCount);
        }
        unbind();
    }

} // namespace bhs::rendering