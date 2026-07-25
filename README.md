# 🌌 Simulation Of Black Hole

> An educational C++ and OpenGL project that builds a real-time black hole simulator from first principles, combining computer graphics, numerical methods, and simplified general relativity.

This repository is not just a black hole visualization.

The goal of this project is to explore how black holes and relativistic visual effects can be simulated in real time while learning the mathematics, physics, and computer graphics concepts behind every implementation.

Every milestone introduces a new feature together with the theory and computational techniques used to build it.

> 🚧 **Work in Progress**
>
> The simulator is actively under development. The current implementation includes the core rendering foundation, procedural starfield, Schwarzschild event horizon, photon sphere visualization, environment cubemap baking, and a GPU-based gravitational lensing pipeline.

---

# 🎯 Project Goals

This project aims to explore and teach:

- Modern C++
- C++20
- OpenGL Rendering Pipeline
- GLSL Shader Programming
- Linear Algebra for Computer Graphics
- 3D Camera Mathematics
- Mesh Generation
- Numerical Methods
- Schwarzschild Geometry
- General Relativity Concepts
- Gravitational Lensing
- Numerical Integration
- Real-Time Graphics Programming
- GPU-based Simulation Techniques

Instead of jumping directly into advanced visual effects, the project is developed incrementally.

Each milestone builds upon the previous rendering and physics systems.

---

# 📚 Learning Roadmap

| Milestone | Topic | Status |
|---|---|---|
| ✅ Milestone 1 | Rendering Pipeline & Procedural UV Sphere | Complete |
| ✅ Milestone 2 | Procedural Starfield | Complete |
| ✅ Milestone 3 | Event Horizon / Schwarzschild Radius | Complete |
| ✅ Milestone 4 | Photon Sphere | Complete |
| ✅ Milestone 5 | Gravitational Lensing & Schwarzschild Geodesic Ray Tracing | Complete |
| ⏳ Milestone 6 | Accretion Disk & Relativistic Effects | Planned |
| ⏳ Milestone 7 | Advanced Light Ray Bending | Planned |
| ⏳ Milestone 8 | Particle System | Planned |
| ⏳ Milestone 9 | Interactive Physics Controls | Planned |
| ⏳ Milestone 10 | Complete Educational Black Hole Simulator | Planned |

---

# 🧠 Physics & Mathematics

Throughout the project, the simulator explores concepts including:

- Euclidean Geometry
- Vector Mathematics
- Matrix Transformations
- Perspective Projection
- Coordinate Spaces
- Uniform Sphere Sampling
- Schwarzschild Radius
- Photon Sphere
- Orbital Mechanics
- Numerical Integration
- Runge-Kutta Integration
- Simplified General Relativity
- Gravitational Lensing
- Null Geodesics

## Schwarzschild Radius

The event horizon radius of a non-rotating black hole is described by:

\[
R_s = \frac{2GM}{c^2}
\]

where:

- \(G\) = gravitational constant
- \(M\) = black hole mass
- \(c\) = speed of light

---

## Photon Sphere

For a Schwarzschild black hole, the photon sphere radius is:

\[
R_p = \frac{3GM}{c^2}
\]

This is the region where photons can theoretically orbit the black hole in unstable circular trajectories.

---

## Critical Impact Parameter

The gravitational lensing implementation also uses the critical impact parameter associated with the Schwarzschild photon sphere:

\[
b_{\text{crit}} = 3\sqrt{3}M
\]

in geometric units.

Rays approaching this critical value can undergo strong gravitational deflection and multiple windings around the black hole before escaping.

---

# 🌌 Current Features

### Rendering

- ✅ GLFW Window
- ✅ OpenGL Rendering
- ✅ Modern Shader Pipeline
- ✅ OpenGL 4.6
- ✅ GLSL Shaders
- ✅ FPS Camera
- ✅ ImGui Debug Panel
- ✅ Reusable Mesh System
- ✅ Procedural UV Sphere
- ✅ Procedural Starfield

### Black Hole Physics & Visualization

- ✅ Schwarzschild Event Horizon
- ✅ Schwarzschild Radius Calculation
- ✅ Photon Sphere Visualization
- ✅ Gravitational Lensing Pipeline
- ✅ Environment Cubemap Generation
- ✅ Framebuffer-based Cubemap Baking
- ✅ Schwarzschild Geodesic Ray Integration
- ✅ Numerical Runge-Kutta Integration
- ✅ Capture / Escape Ray Classification
- ✅ Critical Impact Parameter Handling
- 🚧 Relativistic Accretion Disk
- 🚧 Redshift and Doppler Effects
- 🚧 Advanced Relativistic Effects

---

# 🔭 Gravitational Lensing

Milestone 5 introduces a GPU-based gravitational lensing pipeline.

The rendering process consists of several stages:

1. Generate a procedural starfield.
2. Bake the starfield into a cubemap using framebuffer rendering.
3. Generate a viewing ray for each screen pixel.
4. Integrate the ray through a simplified Schwarzschild spacetime.
5. Determine whether the ray:
   - Falls into the event horizon, or
   - Escapes the black hole's gravitational field.
6. For escaping rays, calculate the final escape direction.
7. Sample the environment cubemap using the resulting direction.
8. Produce the distorted view of the background sky.

Conceptually:

```text
Camera Ray
    │
    ▼
Schwarzschild Geodesic Integration
    │
    ├── Captured by Black Hole
    │       │
    │       ▼
    │     Black
    │
    └── Escapes
            │
            ▼
      Final Escape Direction
            │
            ▼
      Environment Cubemap
            │
            ▼
      Distorted Starfield
