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
        void upload(const MeshData& data);

        // Releases the GPU buffers immediately. Safe to call multiple times.
        void release();

        void bind() const;
        void unbind() const;

        // Binds and issues the indexed draw call.
        void draw() const;

    private:
        unsigned int m_vao = 0;
        unsigned int m_vbo = 0;
        unsigned int m_ebo = 0;
        int m_indexCount = 0;
    };

} // namespace bhs::rendering