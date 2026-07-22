# Black Hole Simulator ITS NOT DONE YET

A long-term C++ project for real-time 3D visualization of black hole physics: event horizon, Schwarzschild radius, photon sphere, accretion disk, gravitational lensing, and relativistic effects.

**Current status:** Foundation build — window, OpenGL loop, FPS camera, ImGui debug panel, and modular architecture. Physics and visual effects are not implemented yet.

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

## Build (Visual Studio 2022)

From the project root:

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
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
