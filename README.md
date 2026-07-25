# Black Hole Simulation

A real-time interactive black hole simulation and visualization project built with **C++**, **OpenGL**, and **GLSL**.

The project aims to progressively simulate and visualize the physics and visual effects surrounding a Schwarzschild black hole, including the event horizon, photon sphere, gravitational lensing, and eventually more advanced relativistic effects.

---

## Project Status

**Current Progress: Milestone 5 — Gravitational Lensing**

The project currently includes the foundational black hole geometry and a GPU-based Schwarzschild gravitational lensing pipeline.

### Milestones

| Milestone | Feature | Status |
|---|---|---|
| M1 | Core Rendering Foundation | ✅ Complete |
| M2 | Event Horizon | ✅ Complete |
| M3 | Black Hole / Event Horizon Visualization | ✅ Complete |
| M4 | Photon Sphere | ✅ Complete |
| M5 | Gravitational Lensing | ✅ Complete |
| M6 | Redshift / Doppler Effects | ⏳ Planned |
| M7 | Advanced Relativistic Effects | ⏳ Planned |
| M8 | Performance Optimization | ⏳ Planned |
| M9 | Visualization / Debugging Improvements | ⏳ Planned |
| M10 | Final Simulation / Release | ⏳ Planned |

---

## Current Features

### Black Hole Geometry

- Schwarzschild black hole model
- Event horizon visualization
- Photon sphere visualization
- Schwarzschild radius derived from the physics module
- Photon sphere radius derived from the existing Schwarzschild geometry
- Central black hole representation

### Gravitational Lensing

Milestone 5 introduces a GPU-based gravitational lensing pipeline based on the Schwarzschild null geodesic equation.

The current implementation includes:

- Fullscreen triangle rendering infrastructure
- Environment cubemap generation
- Framebuffer abstraction
- Cubemap-based background environment
- Schwarzschild geodesic ray integration
- GPU-side ray tracing through the gravitational field
- Black hole shadow / capture region
- Strong-field gravitational lensing
- Near-critical photon trajectories
- Photon-sphere-related lensing behavior
- Near-radial ray handling
- Escape and capture classification
- Final escaped ray direction reconstruction
- Debug visualization modes

The gravitational lensing implementation uses the Schwarzschild orbital equation:

```text
d²u/dφ² + u = 3Mu²

where:

u = 1/r

The implementation uses numerical integration to determine whether a photon escapes the gravitational field or is captured by the black hole.

Rendering Pipeline

The current rendering architecture contains two primary paths.

Existing M1–M4 Rendering Path

The original rendering path remains available and includes the existing:

Starfield
Black hole visualization
Event horizon
Photon sphere wireframe
Debug geometry

This path is preserved and can be used independently from the gravitational lensing pipeline.

Milestone 5 Lensing Path

The lensing pipeline follows this general process:

Camera Ray
    ↓
Fullscreen Triangle
    ↓
Ray Reconstruction
    ↓
Ray-to-Orbital-Plane Conversion
    ↓
Schwarzschild Geodesic Integration
    ↓
Capture / Escape Classification
    ↓
Asymptotic Escaped Direction
    ↓
Environment Cubemap Lookup
    ↓
Final Pixel

The environment cubemap is generated using the existing starfield rendering system and is then sampled by the lensing shader using the final escaped ray direction.

Physics

The simulation currently uses a Schwarzschild black hole model.

The primary physical quantities include:

Rs = 2M

where:

Rs = Schwarzschild radius
M = black hole mass

The photon sphere radius is:

Rp = 3M

or equivalently:

Rp = 1.5Rs

The critical impact parameter is:

b_crit = 3√3M

This separates escaping photon trajectories from captured trajectories in the Schwarzschild geometry.

Gravitational Lensing Implementation

The ray-tracing system uses the conserved impact parameter and the orbital equation to integrate photon trajectories.

The initial ray is converted from the 3D world-space camera ray into a 2D orbital plane.

The orbital plane is defined by:

r̂ = normalize(C - O)

and the initial ray direction:

D

The plane normal is:

n = normalize(cross(r̂, D))

An in-plane basis is then constructed:

e_r   = r̂
e_phi = cross(n, e_r)

The orientation is selected so that the initial tangential component satisfies:

D_phi >= 0

The impact parameter is derived from the locally measured ray direction:

b = r0 * D_phi / sqrt(f(r0))

where:

f(r) = 1 - Rs/r

The initial orbital derivative is determined from the Schwarzschild first integral.

Near-radial rays are handled separately because the orbital-plane construction becomes degenerate as:

b → 0

For non-radial rays, the system numerically integrates the orbital equation and classifies the trajectory as either:

Captured

or:

Escaped

Escaping rays are reconstructed into a final world-space direction and used to sample the environment cubemap.

Validation

The Milestone 5 implementation has been mathematically and numerically validated against several known properties of Schwarzschild null geodesics.

Flat-Space Limit

For:

M = 0

the orbital equation reduces to:

u'' + u = 0

which corresponds to straight-line photon trajectories in Euclidean space.

The implementation was tested against:

Radial outward rays
Radial inward rays
Tangential rays
Generic outward rays
Generic inward rays

The resulting escaped directions reproduce the expected straight-line geometry within numerical integration tolerance.

Weak-Field Limit

For large impact parameters:

b >> b_crit

the numerical deflection approaches the expected weak-field result:

Δθ ≈ 4M/b

This relation is used as a validation check for the numerical integrator and is not used as the rendering method.

Critical Impact Parameter

The critical impact parameter is:

b_crit = 3√3M

and is related to the photon sphere by:

b_crit = Rp / sqrt(f(Rp))

with:

Rp = 3M

This gives:

b_crit = 3√3M

The numerical capture/escape boundary converges toward this critical value.

Near-Critical Behavior

As:

b → b_crit

photon trajectories can wind around the photon sphere for an increasing number of orbital revolutions before either escaping or being captured.

This produces the strong-field gravitational lensing behavior expected around a Schwarzschild black hole.

Controls

The project currently includes an ImGui-based debug interface.

The lensing pipeline can be enabled or disabled independently from the existing rendering path.

Available debugging functionality includes:

Lensing pass toggle
Lensing debug visualization modes
Camera controls
Runtime rendering diagnostics

The original M1–M4 rendering path remains available when the lensing pass is disabled.

Technology Stack
C++
OpenGL 3.3 Core
GLSL
GLFW
GLAD
GLM
Dear ImGui
CMake
Visual Studio / MSVC
Project Structure
BlackHoleSimulation/
│
├── assets/
│   └── shaders/
│       ├── lensing.vert
│       ├── lensing.frag
│       └── ...
│
├── docs/
│   └── ARCHITECTURE.md
│
├── src/
│   ├── Camera/
│   ├── Core/
│   ├── Physics/
│   ├── Rendering/
│   │   ├── EnvironmentBaker.cpp
│   │   ├── EnvironmentBaker.h
│   │   ├── Framebuffer.cpp
│   │   ├── Framebuffer.h
│   │   ├── Renderer.cpp
│   │   └── Renderer.h
│   │
│   └── UI/
│
├── CMakeLists.txt
└── README.md
Architecture

The project is structured around a separation between:

Physics
Camera
Rendering
Shader-based simulation
UI / debugging

The physics module provides the Schwarzschild parameters used by the rendering system.

The renderer is responsible for:

Environment cubemap generation
Fullscreen lensing pass
Shader management
Rendering state
Integration of the physical parameters into the GPU pipeline

The detailed mathematical derivation and architecture documentation can be found in:

docs/ARCHITECTURE.md
Roadmap
Completed
 Core rendering foundation
 Event horizon
 Photon sphere
 Fullscreen triangle rendering infrastructure
 Environment cubemap generation
 Framebuffer abstraction
 Schwarzschild ray-to-orbital-plane conversion
 Schwarzschild null geodesic integration
 Ray capture / escape classification
 Gravitational lensing
 Black hole shadow formation
 Near-critical photon trajectory handling
Planned
 Redshift effects
 Doppler effects
 Additional relativistic visual effects
 Advanced visualization modes
 Performance optimization
 Improved debugging tools
 Final presentation / release build
Milestone 5

Status: Complete

Milestone 5 establishes the first physically-based gravitational lensing implementation in the project.

The simulation now goes beyond simply rendering a black hole model and begins tracing photon trajectories through Schwarzschild spacetime.

The implementation is designed to serve as the foundation for future relativistic rendering features.

License

This project is currently under development.


