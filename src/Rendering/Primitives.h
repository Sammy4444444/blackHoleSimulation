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

    // Position-only point cloud of `count` stars, uniformly distributed over
    // the surface of a sphere of the given radius (correct uniform-sphere
    // sampling, not naive lat/long angles, to avoid polar clustering).
    // Deterministic for a given seed. Returns MeshData with empty indices —
    // there is no shared-vertex topology between disconnected points.
    // Milestone 8, Phase 1: default point count raised from 5000 to 10000
    // for a visibly denser sky. Independent rendering-quality tweak only --
    // same uniform-sphere-surface sampling, same radius/seed, no change to
    // black-hole physics or the lensing pipeline that samples this once
    // baked into the environment cubemap.
    MeshData createStarfieldData(int count = 10000, float radius = 500.0f, unsigned int seed = 1337);

} // namespace bhs::rendering