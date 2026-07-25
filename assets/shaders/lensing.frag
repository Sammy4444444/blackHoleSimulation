#version 330 core

// Milestone 5 — final: physically correct Schwarzschild gravitational
// lensing.
//
// Pipeline per pixel:
//   1. Reconstruct the world-space camera ray direction D (Phase 3, unchanged).
//   2. Build the ray's orbital plane around the black hole (origin) and
//      express D in that plane's (e_r, e_phi) basis.
//   3. Derive the impact parameter b and the initial du/dphi from the exact
//      Schwarzschild relations.
//   4. Integrate the null-geodesic orbit equation d^2u/dphi^2 + u = 3*M*u^2
//      (u = 1/r) forward in phi with fixed-step RK4, terminating on capture
//      (r <= Rs), escape (r >= kEscapeRadius), or the winding safety cutoff.
//   5. Reconstruct the asymptotic escape direction from the final (u, v, phi)
//      state and sample the baked environment cubemap with it.
//
// The near-radial case (ray's orbital plane is numerically undefined because
// the ray points almost exactly at/away from the black hole) is handled in
// closed form instead of running the ODE, since gravity does not deflect an
// exactly radial null geodesic's direction -- only whether it reaches Rs.
//
// See docs/ARCHITECTURE.md for the full derivation and the numerical
// validation performed against this integrator (M=0 straight-line limit,
// b_crit boundary, weak-field 4M/b deflection).

in vec2 vScreenUV;

uniform mat4 uInvProjection;
uniform mat4 uInvView;
uniform vec3 uCameraPos;   // world-space camera position; black hole is at the world origin.
uniform float uMass;       // M, geometrized units (G = c = 1). See PhysicsWorld::mass().
uniform float uSchwarzschildRadius; // Rs = 2M. See PhysicsWorld::schwarzschildRadius().
uniform samplerCube uEnvironment;

// Debug modes (unchanged from Phase 3, still useful for isolating pipeline
// stages from lensing-physics correctness):
//   0 = normal — full geodesic lensing (this milestone's actual output).
//   1 = flat solid color — proves the branch/pass is executing at all,
//       independent of matrices, cubemap, or integrator correctness.
//   2 = raw reconstructed world-ray direction as RGB — proves ray
//       reconstruction (uInvProjection/uInvView) is producing sane,
//       varying-across-the-screen values, upstream of any lensing math.
uniform int uDebugMode;

out vec4 FragColor;

// ---------------------------------------------------------------------
// Named numerical-integration constants (all explicit and documented, per
// Milestone 5's requirement that no unexplained magic numbers govern the
// integrator's behavior or its termination).
// ---------------------------------------------------------------------

// Fixed RK4 step in phi (radians). Smaller = more accurate, more expensive.
// Chosen (together with kMaxPhiSteps below) so the capture/escape boundary
// resolves to within ~1e-5 of the analytic b_crit = 3*sqrt(3)*M at a
// representative camera distance -- see docs/ARCHITECTURE.md validation.
const float kPhiStep = 0.02;

// Maximum number of RK4 steps a single ray is integrated for. This is the
// hard, deterministic bound that prevents an uncontrolled loop: no ray can
// ever run longer than kMaxPhiSteps iterations, regardless of how close its
// impact parameter is to the critical value b_crit. Combined with
// kPhiStep, this caps total angular travel at
// kMaxPhiSteps * kPhiStep = 60 radians (~9.5 full windings around the
// photon sphere) before a ray that has neither been captured nor escaped
// is resolved by the winding-cutoff rule below.
const int kMaxPhiSteps = 3000;

// Below this |cross(rhat, D)| magnitude (rhat, D both unit vectors, so this
// is ~sin of the angle between them), constructing the orbital-plane normal
// by normalizing that cross product is numerically unstable. The ray is
// instead treated as exactly radial (b ~= 0) via the closed-form case
// below. The threshold is set two to three orders of magnitude above
// float32 cross-product noise (~1e-6) for unit-vector inputs, and is far
// below any angular region large enough to be visually distinguishable in
// the rendered image.
const float kNearRadialThreshold = 1e-4;

// Radius beyond which a photon is considered to have reached the "sky at
// infinity" that the baked cubemap represents. Must be large relative to
// both Rs and the camera's typical distance from the black hole so that by
// the time a trajectory is classified as escaped, it has already
// straightened out to (numerically) its true asymptotic direction -- see
// the M=0 and weak-field validation in docs/ARCHITECTURE.md, which
// quantifies the residual direction error introduced by using a large but
// finite radius here instead of literal infinity.
const float kEscapeRadius = 2000.0;

// Stand-in for "1/Rs" when Rs <= 0 (no black hole / M == 0), so the capture
// test `u >= invRs` below is never satisfied by any finite u this
// integrator produces. Deliberately not computed as a literal 1.0/0.0:
// some GLSL compilers fold or reject literal division by zero even inside
// an untaken branch.
const float kNoHorizonInvRs = 3.402823e38;

void main() {
    if (uDebugMode == 1) {
        FragColor = vec4(1.0, 0.0, 1.0, 1.0); // unmistakable magenta
        return;
    }

    // vScreenUV in [0,1] (see lensing.vert) -> NDC in [-1,1].
    vec2 ndc = vScreenUV * 2.0 - 1.0;

    // Unproject to view space. The chosen NDC z (-1, the near plane) is
    // arbitrary: the camera sits at the view-space origin, so the direction
    // from the origin through any point along this pixel's ray is identical
    // regardless of which depth is picked here -- only the direction matters.
    vec4 viewSpace = uInvProjection * vec4(ndc, -1.0, 1.0);
    viewSpace /= viewSpace.w;

    // Rotate into world space. w = 0.0 deliberately drops uInvView's
    // translation column: this transforms a direction, not a position, so
    // camera position must not shift it.
    vec3 worldDir = normalize((uInvView * vec4(viewSpace.xyz, 0.0)).xyz);

    if (uDebugMode == 2) {
        FragColor = vec4(worldDir * 0.5 + 0.5, 1.0); // [-1,1] -> [0,1] so it's visible
        return;
    }

    // --- Ray reconstruction and orbital-plane setup ---------------------
    vec3 C = uCameraPos;
    vec3 D = worldDir;
    float M = uMass;
    float Rs = uSchwarzschildRadius;

    float r0 = length(C);

    // Degenerate: camera sits exactly at the black hole center. Not
    // physically meaningful (and not reachable in normal use -- the event
    // horizon is solid geometry in the M1-M4 path) but guarded rather than
    // dividing by r0 == 0 below.
    if (r0 < 1e-6) {
        FragColor = texture(uEnvironment, D);
        return;
    }

    vec3 rhat = C / r0;
    vec3 crossRD = cross(rhat, D);
    float crossMag = length(crossRD);

    bool captured = false;
    vec3 finalDir = D;

    if (crossMag < kNearRadialThreshold) {
        // --- Near-radial / b ~= 0 closed-form case -----------------------
        // No orbital plane can be stably constructed this close to exactly
        // radial motion, but none is needed: an exactly radial null
        // geodesic is not deflected in direction by a spherically
        // symmetric spacetime, only advanced or delayed in time (which
        // this renderer does not model). So the only question is capture
        // vs. escape, not a final direction.
        float Dr = dot(D, rhat);
        if (Dr < 0.0 && Rs > 0.0) {
            // Moving inward with a horizon present: radial infall always
            // reaches r = Rs (there is no angular momentum to create a
            // centrifugal barrier), so it is captured.
            captured = true;
        } else {
            // Moving outward (or Rs == 0, i.e. no black hole to fall into):
            // escapes undeflected along the original ray direction.
            finalDir = D;
        }
    } else {
        // --- General case: orbital-plane basis ---------------------------
        vec3 n = crossRD / crossMag;
        vec3 e_r = rhat;
        vec3 e_phi = cross(n, e_r);

        float Dr = dot(D, e_r);
        float Dphi = dot(D, e_phi);

        // Orient the basis so phi increases in the ray's direction of
        // travel; angular momentum (and hence its sign) is conserved along
        // the whole geodesic, so this fixes the winding direction for the
        // entire integration once, up front.
        if (Dphi < 0.0) {
            n = -n;
            e_phi = -e_phi;
            Dphi = -Dphi;
        }

        float u0 = 1.0 / r0;
        float f0 = 1.0 - Rs * u0; // Schwarzschild lapse f(r0).

        if (f0 <= 0.0) {
            // Camera at or inside the horizon -- not physically reachable
            // in normal play (the horizon is solid M1-M4 geometry), but
            // guarded rather than taking sqrt() of a non-positive number
            // below.
            FragColor = vec4(0.0, 0.0, 0.0, 1.0);
            return;
        }

        // Impact parameter, from the exact Schwarzschild relation.
        float b = r0 * Dphi / sqrt(f0);

        // Initial du/dphi. The radicand is exactly >= 0 analytically (it
        // reduces to (u0*sqrt(f0)*Dr/Dphi)^2, see docs/ARCHITECTURE.md) but
        // is clamped defensively against floating-point noise pushing it
        // fractionally negative right at Dr ~= 0.
        float radicand = max(1.0 / (b * b) - u0 * u0 * f0, 0.0);
        float dudphi = -sign(Dr) * sqrt(radicand);

        float u = u0;
        float v = dudphi;
        float phi = 0.0;

        bool escaped = false;
        float invRs = (Rs > 0.0) ? (1.0 / Rs) : kNoHorizonInvRs; // no horizon -> never captured by radius.

        // --- Fixed-step RK4 integration of the orbit equation -----------
        //   du/dphi = v
        //   dv/dphi = 3*M*u^2 - u
        for (int i = 0; i < kMaxPhiSteps; ++i) {
            vec2 s0 = vec2(u, v);

            vec2 k1 = vec2(s0.y, 3.0 * M * s0.x * s0.x - s0.x);
            vec2 s1 = s0 + 0.5 * kPhiStep * k1;

            vec2 k2 = vec2(s1.y, 3.0 * M * s1.x * s1.x - s1.x);
            vec2 s2 = s0 + 0.5 * kPhiStep * k2;

            vec2 k3 = vec2(s2.y, 3.0 * M * s2.x * s2.x - s2.x);
            vec2 s3 = s0 + kPhiStep * k3;

            vec2 k4 = vec2(s3.y, 3.0 * M * s3.x * s3.x - s3.x);

            vec2 result = s0 + (kPhiStep / 6.0) * (k1 + 2.0 * k2 + 2.0 * k3 + k4);
            u = result.x;
            v = result.y;
            phi += kPhiStep;

            if (u >= invRs) {
                captured = true;
                break;
            }
            if (u <= 1.0 / kEscapeRadius) {
                escaped = true;
                break;
            }
        }

        if (!captured && !escaped) {
            // Exceeded kMaxPhiSteps (kMaxTotalPhi = 60 rad) without
            // resolving. This only happens for b within numerical whisker
            // of b_crit: the true fate of such a ray is to asymptote onto
            // the unstable circular photon orbit at the photon sphere,
            // which is itself the boundary of the black hole's shadow.
            // Treating the cutoff as capture is therefore the physically
            // consistent choice, not an arbitrary one.
            captured = true;
        }

        if (!captured) {
            // Final asymptotic escape direction, per the derivation in
            // docs/ARCHITECTURE.md: as r -> infinity the trajectory tangent
            // approaches cos(phi)*e_r + sin(phi)*e_phi.
            finalDir = cos(phi) * e_r + sin(phi) * e_phi;
        }
    }

    if (captured) {
        // Black hole shadow: the event-horizon capture region reads as
        // pure black, matching the solid black horizon mesh drawn in the
        // M1-M4 path.
        FragColor = vec4(0.0, 0.0, 0.0, 1.0);
        return;
    }

    FragColor = texture(uEnvironment, normalize(finalDir));
}
