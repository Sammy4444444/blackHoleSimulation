#pragma once

#include <vector>

namespace bhs::rendering {

    // Plain CPU-side geometry (position-only: 3 floats per vertex). No GL calls
    // happen here; Mesh::upload() consumes this to create GPU buffers.
    struct MeshData {
        std::vector<float> vertices;
        std::vector<unsigned int> indices;
    };

    // Unit-ish reference cube, centered at the origin (matches the geometry
    // previously hardcoded in Renderer::createSceneGeometry).
    MeshData createCubeData();

    // Procedural UV sphere using indexed geometry, poles along +Y/-Y to match
    // this project's Y-up camera convention.
    MeshData createUVSphereData(float radius = 0.5f, int sectorCount = 36, int stackCount = 18);

} // namespace bhs::rendering