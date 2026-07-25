#pragma once

namespace bhs::rendering {

    class Shader;
    class Mesh;

    // Milestone 5, Phase 2: bakes the existing M2 starfield into a cubemap
    // representing the sky at infinity, as seen from the origin looking
    // outward in all six axis-aligned directions.
    //
    // This is a one-time startup operation, not a per-frame system. It
    // reuses the existing starfield Mesh and Shader exactly as already
    // built by Renderer::initialize() -- no star-generation logic, vertex
    // data, or shader source is duplicated here. The resulting cubemap is
    // not yet sampled anywhere (that is Phase 3's job); Phase 2 only
    // produces it and verifies it was built correctly.
    class EnvironmentBaker {
    public:
        EnvironmentBaker() = default;
        ~EnvironmentBaker();

        EnvironmentBaker(const EnvironmentBaker&) = delete;
        EnvironmentBaker& operator=(const EnvironmentBaker&) = delete;

        // Renders `starMesh` (drawn with `starShader`, exactly as Renderer's
        // normal starfield pass does) into all six faces of a newly created
        // cubemap texture. Called once at startup, after the starfield Mesh
        // and Shader already exist. Safe to call only once; a second call
        // on an already-baked instance is a no-op (with a warning) so
        // callers cannot accidentally re-bake every frame.
        void bake(Shader& starShader, const Mesh& starMesh, unsigned int faceResolution = 1024);

        // Deletes the cubemap texture immediately. Safe to call multiple
        // times.
        void release();

        bool isBaked() const { return m_cubemapTexture != 0; }
        unsigned int environmentMap() const { return m_cubemapTexture; }

    private:
        unsigned int m_cubemapTexture = 0;
    };

} // namespace bhs::rendering
