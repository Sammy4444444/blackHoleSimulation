#include "Rendering/EnvironmentBaker.h"

#include "Rendering/Framebuffer.h"
#include "Rendering/Shader.h"
#include "Rendering/Mesh.h"
#include "Core/Log.h"

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <array>
#include <string>

namespace bhs::rendering {
    using bhs::core::Log;

    namespace {
        struct CubeFace {
            unsigned int target;
            glm::vec3 direction;
            glm::vec3 up;
            const char* name;
        };

        // Standard OpenGL cubemap face convention (the same orientation
        // used for e.g. point-light shadow cubemaps): each face looks
        // straight down one axis, with the listed up-vector, so all six
        // faces agree on a consistent handedness at their shared edges.
        const std::array<CubeFace, 6> kFaces = {{
            { GL_TEXTURE_CUBE_MAP_POSITIVE_X, glm::vec3( 1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f), "+X" },
            { GL_TEXTURE_CUBE_MAP_NEGATIVE_X, glm::vec3(-1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f), "-X" },
            { GL_TEXTURE_CUBE_MAP_POSITIVE_Y, glm::vec3( 0.0f,  1.0f,  0.0f), glm::vec3(0.0f,  0.0f,  1.0f), "+Y" },
            { GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, glm::vec3( 0.0f, -1.0f,  0.0f), glm::vec3(0.0f,  0.0f, -1.0f), "-Y" },
            { GL_TEXTURE_CUBE_MAP_POSITIVE_Z, glm::vec3( 0.0f,  0.0f,  1.0f), glm::vec3(0.0f, -1.0f,  0.0f), "+Z" },
            { GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, glm::vec3( 0.0f,  0.0f, -1.0f), glm::vec3(0.0f, -1.0f,  0.0f), "-Z" },
        }};

        // Matches the background clear color of the normal M1-M4 scene
        // (Renderer::render) so the baked sky reads as a continuation of
        // the same space rather than a visibly different shade.
        constexpr float kClearR = 0.02f;
        constexpr float kClearG = 0.02f;
        constexpr float kClearB = 0.05f;
    } // namespace

    EnvironmentBaker::~EnvironmentBaker() {
        release();
    }

    void EnvironmentBaker::bake(Shader& starShader, const Mesh& starMesh, unsigned int faceResolution) {
        if (m_cubemapTexture != 0) {
            Log::warn("EnvironmentBaker::bake() called on an already-baked instance; ignoring.");
            return;
        }

        // --- Create and allocate the cubemap ---
        glGenTextures(1, &m_cubemapTexture);
        glBindTexture(GL_TEXTURE_CUBE_MAP, m_cubemapTexture);

        for (const CubeFace& face : kFaces) {
            glTexImage2D(face.target, 0, GL_RGBA8,
                static_cast<GLsizei>(faceResolution), static_cast<GLsizei>(faceResolution),
                0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        }

        // Milestone 8, Phase 1: trilinear (mipmapped) filtering instead of
        // plain bilinear. This is a pure sampling-quality improvement for
        // the environment cubemap -- it changes nothing about which
        // direction is sampled (that's still exactly finalDir from the
        // unmodified M5 geodesic integration in lensing.frag), only how
        // that sample is filtered, reducing aliasing/shimmer on distant
        // starfield detail versus the single-level bilinear filtering used
        // through M7. Mipmaps themselves are generated below, once all six
        // faces have been rendered into level 0.
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

        // --- Preserve state this bake needs to change temporarily ---
        GLint previousViewport[4] = {};
        glGetIntegerv(GL_VIEWPORT, previousViewport);
        const GLboolean depthTestWasEnabled = glIsEnabled(GL_DEPTH_TEST);

        // The FBO used for the bake has no depth attachment (color-only,
        // per the Milestone 5 architecture -- an FBO is only needed for
        // this one-time bake, not the per-frame lensing pass). Depth
        // testing against a nonexistent depth buffer is meaningless here,
        // so it is disabled for the duration of the bake and restored
        // afterward.
        glDisable(GL_DEPTH_TEST);

        Framebuffer fbo;
        fbo.create();
        fbo.bind();

        glViewport(0, 0, static_cast<GLsizei>(faceResolution), static_cast<GLsizei>(faceResolution));

        // 90-degree FOV with a 1:1 aspect ratio is exactly one cube face's
        // field of view -- the standard setup for baking a cubemap face by
        // face.
        const glm::mat4 projection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 1000.0f);

        starShader.bind();
        const int viewLoc = glGetUniformLocation(starShader.id(), "uView");
        const int projLoc = glGetUniformLocation(starShader.id(), "uProjection");
        glUniformMatrix4fv(projLoc, 1, GL_FALSE, &projection[0][0]);

        bool allFacesComplete = true;

        for (const CubeFace& face : kFaces) {
            fbo.attachCubemapFace(m_cubemapTexture, face.target);

            if (!fbo.isComplete()) {
                allFacesComplete = false;
                Log::warn(std::string("EnvironmentBaker: cubemap face ") + face.name + " framebuffer incomplete; skipping.");
                continue;
            }

            glClearColor(kClearR, kClearG, kClearB, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);

            // Eye is the origin: the starfield represents the sky at
            // infinity, and the normal M1-M4 starfield pass already strips
            // camera translation from its view matrix for the same reason
            // (see Renderer::render), so baking from the origin reproduces
            // exactly what that pass shows regardless of where the play
            // camera actually sits.
            const glm::mat4 view = glm::lookAt(glm::vec3(0.0f), face.direction, face.up);
            glUniformMatrix4fv(viewLoc, 1, GL_FALSE, &view[0][0]);

            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            glDepthMask(GL_FALSE);

            starMesh.draw();

            if (face.target == GL_TEXTURE_CUBE_MAP_POSITIVE_X) {
                // One-time debug sanity check, not permanent debug
                // infrastructure: confirms this face actually got
                // rasterized into by reading back a single center pixel.
                // Cheap (1x1 readback) and runs only once at startup.
                unsigned char pixel[4] = {};
                glReadPixels(static_cast<GLint>(faceResolution) / 2, static_cast<GLint>(faceResolution) / 2,
                    1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixel);
                Log::info("EnvironmentBaker: +X face center pixel sample = (" +
                    std::to_string(pixel[0]) + ", " + std::to_string(pixel[1]) + ", " +
                    std::to_string(pixel[2]) + ", " + std::to_string(pixel[3]) + ")");
            }

            glDepthMask(GL_TRUE);
            glDisable(GL_BLEND);
        }

        starShader.unbind();
        Framebuffer::unbind();

        // Milestone 8, Phase 1: build the mipmap chain now that all six
        // faces are populated at level 0. Must happen after the face loop
        // (mipmap generation needs complete base-level data) and while the
        // cubemap is still bound. Combined with the GL_LINEAR_MIPMAP_LINEAR
        // min-filter set above, this gives trilinear-filtered environment
        // sampling in lensing.frag.
        glBindTexture(GL_TEXTURE_CUBE_MAP, m_cubemapTexture);
        glGenerateMipmap(GL_TEXTURE_CUBE_MAP);

        // --- Restore state for the caller ---
        glViewport(previousViewport[0], previousViewport[1], previousViewport[2], previousViewport[3]);
        if (depthTestWasEnabled) {
            glEnable(GL_DEPTH_TEST);
        }

        glBindTexture(GL_TEXTURE_CUBE_MAP, 0);

        if (allFacesComplete) {
            Log::info("EnvironmentBaker: baked " + std::to_string(faceResolution) + "x" +
                std::to_string(faceResolution) + " starfield cubemap (all 6 faces complete).");
        } else {
            Log::warn("EnvironmentBaker: cubemap bake finished with at least one incomplete face.");
        }
    }

    void EnvironmentBaker::release() {
        if (m_cubemapTexture != 0) {
            glDeleteTextures(1, &m_cubemapTexture);
            m_cubemapTexture = 0;
        }
    }

} // namespace bhs::rendering
