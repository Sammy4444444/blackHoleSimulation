#include "Rendering/Primitives.h"

#include <cmath>

namespace bhs::rendering {

    namespace {
        constexpr float kPi = 3.14159265358979323846f;
    }

    MeshData createCubeData() {
        // Reference cube at the origin so camera movement is visible immediately.
        // (Moved unchanged from the previous Renderer::createSceneGeometry.)
        MeshData data;

        data.vertices = {
            // positions
            -0.5f, -0.5f, -0.5f,
             0.5f, -0.5f, -0.5f,
             0.5f,  0.5f, -0.5f,
            -0.5f,  0.5f, -0.5f,
            -0.5f, -0.5f,  0.5f,
             0.5f, -0.5f,  0.5f,
             0.5f,  0.5f,  0.5f,
            -0.5f,  0.5f,  0.5f,
        };

        data.indices = {
            0, 1, 2, 2, 3, 0,
            4, 5, 6, 6, 7, 4,
            0, 1, 5, 5, 4, 0,
            2, 3, 7, 7, 6, 2,
            0, 3, 7, 7, 4, 0,
            1, 2, 6, 6, 5, 1,
        };

        return data;
    }

    MeshData createUVSphereData(float radius, int sectorCount, int stackCount) {
        MeshData data;

        // Vertices: one ring per stack, from the north pole (stack 0) to the
        // south pole (stack stackCount), sectorCount+1 vertices per ring (the
        // first and last sector column share the same position so the sphere
        // can be indexed as a regular grid without a wraparound special case).
        for (int stack = 0; stack <= stackCount; ++stack) {
            // Stack angle sweeps from +90 degrees (north pole) to -90 degrees
            // (south pole).
            const float stackAngle = kPi / 2.0f - static_cast<float>(stack) * (kPi / static_cast<float>(stackCount));
            const float y = radius * std::sin(stackAngle);
            const float ringRadius = radius * std::cos(stackAngle);

            for (int sector = 0; sector <= sectorCount; ++sector) {
                const float sectorAngle = static_cast<float>(sector) * (2.0f * kPi / static_cast<float>(sectorCount));

                const float x = ringRadius * std::cos(sectorAngle);
                const float z = ringRadius * std::sin(sectorAngle);

                data.vertices.push_back(x);
                data.vertices.push_back(y);
                data.vertices.push_back(z);
            }
        }

        // Indices: two triangles per quad between consecutive rings/sectors.
        // At the poles, one ring collapses to a single point, which would make
        // one of the two triangles per quad degenerate (zero area); that
        // triangle is skipped below rather than emitted, so each pole becomes a
        // fan of single triangles instead of a ring of degenerate ones.
        const int verticesPerRing = sectorCount + 1;

        for (int stack = 0; stack < stackCount; ++stack) {
            int topRowStart = stack * verticesPerRing;
            int bottomRowStart = (stack + 1) * verticesPerRing;

            for (int sector = 0; sector < sectorCount; ++sector) {
                const unsigned int topLeft = static_cast<unsigned int>(topRowStart + sector);
                const unsigned int topRight = static_cast<unsigned int>(topRowStart + sector + 1);
                const unsigned int bottomLeft = static_cast<unsigned int>(bottomRowStart + sector);
                const unsigned int bottomRight = static_cast<unsigned int>(bottomRowStart + sector + 1);

                // Wound so each triangle is CCW as seen from outside the sphere,
                // matching glFrontFace(GL_CCW) + back-face culling
                // (OpenGLContext.cpp) — otherwise every face would be culled.
                if (stack != 0) {
                    data.indices.push_back(topLeft);
                    data.indices.push_back(topRight);
                    data.indices.push_back(bottomLeft);
                }

                if (stack != stackCount - 1) {
                    data.indices.push_back(topRight);
                    data.indices.push_back(bottomRight);
                    data.indices.push_back(bottomLeft);
                }
            }
        }

        return data;
    }

} // namespace bhs::rendering