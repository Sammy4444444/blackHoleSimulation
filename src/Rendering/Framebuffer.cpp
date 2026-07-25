#include "Rendering/Framebuffer.h"

#include "Core/Log.h"

#include <glad/glad.h>

namespace bhs::rendering {
    using bhs::core::Log;

    Framebuffer::~Framebuffer() {
        release();
    }

    Framebuffer::Framebuffer(Framebuffer&& other) noexcept
        : m_fbo(other.m_fbo) {
        other.m_fbo = 0;
    }

    Framebuffer& Framebuffer::operator=(Framebuffer&& other) noexcept {
        if (this != &other) {
            release();
            m_fbo = other.m_fbo;
            other.m_fbo = 0;
        }
        return *this;
    }

    void Framebuffer::create() {
        if (m_fbo != 0) {
            Log::warn("Framebuffer::create() called on an already-created framebuffer; ignoring.");
            return;
        }
        glGenFramebuffers(1, &m_fbo);
    }

    void Framebuffer::bind() const {
        glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
    }

    void Framebuffer::unbind() {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    void Framebuffer::attachCubemapFace(unsigned int cubemapTexture, unsigned int faceTarget, int mipLevel) {
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, faceTarget, cubemapTexture, mipLevel);
    }

    bool Framebuffer::isComplete() const {
        const GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
        if (status == GL_FRAMEBUFFER_COMPLETE) {
            return true;
        }

        Log::warn("Framebuffer incomplete, status: 0x" + std::to_string(status));
        return false;
    }

    void Framebuffer::release() {
        if (m_fbo != 0) {
            glDeleteFramebuffers(1, &m_fbo);
            m_fbo = 0;
        }
    }

} // namespace bhs::rendering
