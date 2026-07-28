# Black Hole Simulator ITS NOT DONE YET

A long-term C++ project for real-time 3D visualization of black hole physics: event horizon, Schwarzschild radius, photon sphere, accretion disk, gravitational lensing, and relativistic effects.

**Current status:** Milestones 1-8 (Phase 1) implemented — window/OpenGL loop/FPS camera/ImGui foundation, event horizon, photon sphere, full Schwarzschild geodesic gravitational lensing, a physically-motivated accretion disk rendered through the same lensing pipeline, relativistic gravitational redshift + Doppler shading (beaming) of the disk, and an optional per-pixel NxN supersampling/jitter image-quality refinement layered on top of the same lensing pass, all evaluated in the same per-pixel shader pass (see `docs/ARCHITECTURE.md`). The disk, relativistic-effects layer, and supersampling/jitter are all off by default and toggled from the ImGui panel.

### Milestone 8, Phase 1 — Lensing Refinement

Image-quality refinement of the existing M5-M7 lensing pass, with no changes to the underlying Schwarzschild geodesic physics, RK4 integration, capture/escape logic, or disk/relativistic shading:

- **Optional NxN supersampling** (1x1 to 4x4, off by default): the per-pixel ray tracer (`traceRay()` in `assets/shaders/lensing.frag`) is invoked once per subsample, each with its own fractionally-offset ray direction, and the results are averaged.
- **Optional subpixel jitter**: stratified (jittered) sampling of each subsample's position within its grid cell, instead of always sampling at the exact cell center.
- **Trilinear-filtered, mipmapped environment cubemap**: the baked starfield cubemap now builds a full mipmap chain and samples with `GL_LINEAR_MIPMAP_LINEAR`, reducing shimmer/aliasing on distant starfield detail.
- **Denser default starfield** (5,000 → 10,000 points), a small independent rendering-quality adjustment.
- New ImGui panel section ("Milestone 8: Lensing Refinement (Phase 1)") to enable supersampling, choose the grid size, and toggle jitter.

All four new controls default to the pre-M8 (Milestone 7) behavior: supersampling and jitter start OFF, so `traceRay()` runs exactly once per pixel with the same center-of-pixel ray reconstruction used since Phase 3 unless a person explicitly turns supersampling on.

## Architecture

```
SimulationOfBlackHole/
├── CMakeLists.txt          # Root build configuration
├── cmake/
│   └── Dependencies.cmake  # FetchContent: GLFW, GLM, GLAD, ImGui
├── assets/
│   └── shaders/            # GLSL shader sources
└── src/
    ├── main.cpp            # Entry point
    ├── Core/               # Application lifecycle, window, logging, timing
    ├── Rendering/          # OpenGL context, shaders, renderer
    ├── Camera/             # 3D camera and input controller
    ├── Physics/            # Physics stub (future simulation)
    ├── UI/                 # Dear ImGui debug overlay
    └── Assets/             # File loading utilities
```

### Module responsibilities

| Module | Purpose |
|--------|---------|
| **Core** | `Application` main loop, `Window` (GLFW), `Log`, `Timer` |
| **Rendering** | GLAD loader init, `Shader` compilation, `Renderer` draw loop |
| **Camera** | Perspective camera math (GLM), WASD + mouse controls |
| **Physics** | Placeholder `PhysicsWorld` for future GR/orbit simulation |
| **UI** | ImGui overlay (FPS, camera stats, controls) |
| **Assets** | Shader and future texture/mesh loading |

## Requirements

- Windows 11
- Visual Studio 2026 (Desktop development with C++)
- CMake 3.20+
- Git (for FetchContent dependencies)
- OpenGL 3.3+ capable GPU

## Build (Visual Studio 2026)

From the project root:

```powershell
cmake -S . -B build -G "Visual Studio 17 2026" -A x64
cmake --build build --config Release
```

First configure downloads GLFW, GLM, GLAD, and Dear ImGui via CMake FetchContent (requires network).

## Run

```powershell
.\build\Release\BlackHoleSimulator.exe
```

Or open `build\BlackHoleSimulator.sln` in Visual Studio 2022 and press **F5**.

## Controls

| Input | Action |
|-------|--------|
| W / A / S / D | Move forward / left / back / right |
| Q / E | Move down / up |
| Right mouse + drag | Look around |
| Scroll wheel | Zoom (FOV) |
| Esc | Quit |
