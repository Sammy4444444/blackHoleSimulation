#pragma once

namespace bhs::rendering {

    // Minimal RAII wrapper around an OpenGL framebuffer object (FBO).
    //
    // Scope is deliberately narrow: this exists to support the one-time
    // cubemap bake in EnvironmentBaker (Milestone 5, Phase 2), not as a
    // general-purpose multi-attachment/depth-buffer framebuffer class. It
    // supports creating an FBO, binding it, attaching a single cubemap face
    // as the color attachment, checking completeness, and releasing it. Per
    // the Milestone 5 architecture, the per-frame lensing pass renders
    // directly to the default framebuffer and never touches this class.
    class Framebuffer {
    public:
        Framebuffer() = default;
        ~Framebuffer();

        Framebuffer(const Framebuffer&) = delete;
        Framebuffer& operator=(const Framebuffer&) = delete;

        Framebuffer(Framebuffer&& other) noexcept;
        Framebuffer& operator=(Framebuffer&& other) noexcept;

        // Generates the underlying FBO name. Safe to call multiple times;
        // a no-op (with a warning) if already created.
        void create();

        // Binds this FBO as both the read and draw target.
        void bind() const;

        // Binds the default framebuffer (id 0).
        static void unbind();

        // Attaches one face of a cubemap texture as GL_COLOR_ATTACHMENT0.
        // faceTarget is one of GL_TEXTURE_CUBE_MAP_POSITIVE_X .. NEGATIVE_Z.
        // This framebuffer must already be bound when calling this.
        void attachCubemapFace(unsigned int cubemapTexture, unsigned int faceTarget, int mipLevel = 0);

        // Checks GL_FRAMEBUFFER completeness for whatever is currently
        // attached. This framebuffer must already be bound when calling
        // this. Logs a warning with the specific status on failure.
        bool isComplete() const;

        // Deletes the FBO immediately. Safe to call multiple times.
        void release();

        unsigned int id() const { return m_fbo; }

    private:
        unsigned int m_fbo = 0;
    };

} // namespace bhs::rendering
