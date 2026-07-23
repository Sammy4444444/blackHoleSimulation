#pragma once

#include "Rendering/Primitives.h"

namespace bhs::rendering {

    class Mesh {
    public:
        Mesh() = default;
        ~Mesh();

        Mesh(const Mesh&) = delete;
        Mesh& operator=(const Mesh&) = delete;
        Mesh(Mesh&& other) noexcept;
        Mesh& operator=(Mesh&& other) noexcept;

        // Creates (or replaces) the GPU buffers from CPU-side geometry data.
        // primitiveType defaults to GL_TRIANGLES so existing call sites are
        // unaffected. If data.indices is empty, no EBO is created and draw()
        // issues glDrawArrays instead of glDrawElements.
        void upload(const MeshData& data, unsigned int primitiveType = 0x0004 /* GL_TRIANGLES */);

        // Releases the GPU buffers immediately. Safe to call multiple times.
        void release();

        void bind() const;
        void unbind() const;

        // Binds and issues the draw call (indexed or non-indexed, depending on
        // whether an EBO was created in upload()).
        void draw() const;

    private:
        unsigned int m_vao = 0;
        unsigned int m_vbo = 0;
        unsigned int m_ebo = 0;
        int m_indexCount = 0;
        int m_vertexCount = 0;
        unsigned int m_primitiveType = 0x0004 /* GL_TRIANGLES */;
    };

} // namespace bhs::rendering