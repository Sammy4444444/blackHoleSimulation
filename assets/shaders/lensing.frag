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
//
// Milestone 6 adds an accretion disk, modeled as a geometrically thin ring
// lying exactly in the world-space equatorial plane (y = 0), centered on
// the black hole, between uDiskInnerRadius and uDiskOuterRadius. It is
// evaluated *inside* the same per-pixel RK4 loop below rather than as a
// separate screen-space pass, so the disk is subject to the same
// gravitational lensing as the background: a ray whose orbital plane winds
// around the black hole crosses the equatorial plane multiple times, and
// each crossing is tested against the disk's radial extent. This is what
// produces the disk's "wraps around behind the black hole" appearance
// (multiple images of the far side of the disk, visible above/below the
// shadow) rather than a flat unlensed overlay. See docs/ARCHITECTURE.md,
// Milestone 6, for the full geometric derivation and the emission model.
//
// Milestone 7 adds relativistic redshift/Doppler shading on top of the M6
// disk-crossing pipeline, off by default. At the same disk-crossing point
// M6 already resolves (r, and the already-computed orbital-plane basis
// n/e_r/e_phi and impact parameter b), the combined gravitational +
// transverse-Doppler + longitudinal-Doppler redshift factor g for a
// circular-Keplerian disk emitter observed by a static observer is folded
// into the M6 temperature (T_obs = g * T) *before* it enters the existing
// blackbody-color/T^4-flux/tonemap pipeline -- so color shift (redshift/
// blueshift) and relativistic beaming (the g^4 intensity boost baked into
// T_obs^4) both fall out of that one substitution rather than needing a
// separate color/brightness path. See docs/ARCHITECTURE.md, Milestone 7,
// for the full derivation, the finite-camera-distance correction, and the
// numerical guards used near the photon sphere and inner disk edge.

in vec2 vScreenUV;

uniform mat4 uInvProjection;
uniform mat4 uInvView;
uniform vec3 uCameraPos;   // world-space camera position; black hole is at the world origin.
uniform float uMass;       // M, geometrized units (G = c = 1). See PhysicsWorld::mass().
uniform float uSchwarzschildRadius; // Rs = 2M. See PhysicsWorld::schwarzschildRadius().
uniform samplerCube uEnvironment;

// Milestone 6: accretion disk. All in the same world/geometrized units as
// uSchwarzschildRadius. See Renderer's disk accessors and
// docs/ARCHITECTURE.md for the full model.
uniform int uDiskEnabled;
uniform float uDiskInnerRadius;
uniform float uDiskOuterRadius;
uniform float uDiskReferenceTemperature;
uniform float uDiskBrightness;

// Milestone 7: relativistic redshift/Doppler shading, layered on top of the
// Milestone 6 disk. See docs/ARCHITECTURE.md, Milestone 7.
uniform int uRelativisticEnabled;
// Sign of the disk's fixed world-space rotation sense relative to +Y:
// +1.0 = prograde (co-rotating with +Y by the right-hand rule), -1.0 =
// retrograde. Only changes which side of the disk appears blueshifted;
// never changes M5/M6 geometry. See dopplerRedshiftFactor() below.
uniform float uDiskRotationDirection;

// Milestone 8, Phase 1: lensing refinement -- optional per-pixel NxN
// supersampling with optional subpixel jitter/stratified sampling, layered
// on top of the M5-M7 pipeline below without touching its physics. Both
// are off by default (uSupersamplingEnabled == 0), in which case main()
// takes the single-sample path -- exactly one traceRay() call with the
// pixel-center ray already reconstructed the same way Phase 3 always did
// -- so the default rendered image is unchanged from the M7 baseline. See
// docs/ARCHITECTURE.md, Milestone 8, for the sampling-pattern derivation.
uniform vec2 uResolution;          // framebuffer size in pixels; converts the NxN subpixel grid into screen-UV offsets.
uniform int uSupersamplingEnabled; // 0 = off (default): single center sample, byte-for-byte the pre-M8 path.
uniform int uSupersamplingGrid;    // requested NxN grid size; clamped to [1, 4] in-shader regardless of what's uploaded.
uniform int uJitterEnabled;        // 0 = off (default): each subsample sits at its stratified cell's exact center.

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

// ---------------------------------------------------------------------
// Milestone 7 — relativistic redshift/Doppler numerical guards
// ---------------------------------------------------------------------

// Lower bound on |1 - s*Omega*b| (the longitudinal-Doppler denominator in
// dopplerRedshiftFactor() below). This denominator's true zero is the
// physical (Cunningham 1975) infinite-blueshift condition for a photon
// tangent to a co-rotating relativistic circular orbit, which real disks
// approach arbitrarily closely near the photon sphere; clamping its
// magnitude away from zero keeps g finite there instead of dividing by
// (numerical) zero, at the cost of capping -- rather than eliminating --
// the blueshift spike right at that boundary.
const float kMinDopplerDenom = 0.05;

// Hard ceiling on the redshift/Doppler factor g itself, applied after the
// gravitational and Doppler terms (and the finite-camera correction) are
// combined. Without this, kMinDopplerDenom's cap on the denominator alone
// would still let g grow large enough that T_obs^4 in diskEmission()
// could push the tonemap's exp() argument into range that some GPU
// drivers evaluate imprecisely; capping g directly guarantees a bounded,
// driver-independent input to the existing M6 tonemap.
const float kMaxDopplerFactor = 8.0;

// ---------------------------------------------------------------------
// Milestone 6 — accretion disk model
// ---------------------------------------------------------------------

// Shakura-Sunyaev-style radial temperature profile for a thin accretion
// disk with a zero-torque inner boundary condition at r = Rin:
//
//   T(r) = Tref * (Rin/r)^0.75 * (1 - sqrt(Rin/r))^0.25   for r >= Rin
//
// This is the standard steady-state thin-disk temperature *shape*
// (Shakura & Sunyaev 1973): it correctly goes to zero at r = Rin (the
// physical zero-torque condition, not an arbitrary clamp) and falls off
// roughly as r^-0.75 at large r/Rin. Tref (uDiskReferenceTemperature) is a
// prefactor, not the disk's peak temperature -- the (1 - sqrt(Rin/r))^0.25
// factor means the actual maximum of T(r) is reached near r ~= 1.36*Rin
// and is noticeably below Tref (~0.49*Tref at exactly that radius).
//
// What this deliberately does NOT model: the real Shakura-Sunyaev Tref
// depends on accretion rate, viscosity (alpha), and black hole mass via
// Tref ~ (Mdot * M^-2)^0.25 in physical units; none of that is simulated
// here; uDiskReferenceTemperature is instead a single free UI parameter in
// unitless "simulation temperature", standing in for that whole physical
// prefactor. Treat this as a qualitatively correct radial *shape*, not a
// quantitative Kelvin prediction.
float diskTemperature(float r, float innerRadius) {
    float xi = innerRadius / r; // in (0, 1] for r >= innerRadius
    float torqueFactor = max(1.0 - sqrt(xi), 0.0);
    return uDiskReferenceTemperature * pow(xi, 0.75) * pow(torqueFactor, 0.25);
}

// Approximate blackbody-inspired color ramp, NOT a physically integrated
// Planckian-locus/CIE calculation. Real spectral-to-RGB conversion needs an
// integral against the CIE color-matching functions, which is out of scope
// for a per-pixel real-time shader. This instead interpolates between a few
// representative colors (cool dim red -> orange -> yellow-white -> hot
// blue-white) ordered the same way real blackbody hue shifts with rising
// temperature, so the qualitative "hotter = whiter/bluer, cooler =
// dimmer/redder" behavior is preserved even though the exact hues are an
// artistic approximation.
vec3 blackbodyApprox(float tNorm) {
    vec3 cool = vec3(0.35, 0.03, 0.0);    // dim ember red
    vec3 warm = vec3(1.0, 0.35, 0.05);    // orange
    vec3 hot = vec3(1.0, 0.85, 0.55);     // yellow-white
    vec3 veryHot = vec3(0.75, 0.85, 1.0); // blue-white

    vec3 c = mix(cool, warm, smoothstep(0.0, 0.12, tNorm));
    c = mix(c, hot, smoothstep(0.10, 0.30, tNorm));
    c = mix(c, veryHot, smoothstep(0.28, 0.55, tNorm));
    return c;
}

// Milestone 7: combined gravitational + transverse-Doppler + longitudinal-
// Doppler redshift factor g = nu_observed / nu_emitted, for a disk emitter
// on a circular Keplerian test-particle orbit at radius r (Schwarzschild
// equatorial plane), as measured by a *static* observer -- exact closed
// form (Luminet 1979; Cunningham 1975):
//
//   g = sqrt(1 - 3M/r) / (1 - s*Omega(r)*b) / sqrt(f(r0))
//
// where Omega(r) = sqrt(M)/r^1.5 is the material's Keplerian angular
// velocity, b is the photon's impact parameter (conserved along the whole
// geodesic -- already computed once, before the RK4 loop, by the caller),
// s = sign(dot(n, +Y)) * rotationDir encodes whether this particular ray's
// orbital plane winds the same way as the disk's fixed world-space
// rotation sense (co-rotating) or the opposite way (counter-rotating) --
// necessary because n can point along +Y or -Y depending on the ray, while
// the disk itself has one fixed rotation direction in world space -- and
// the final 1/sqrt(f(r0)) converts the sqrt(1-3M/r) term's implicit
// "observer at infinity" normalization to "observer at the camera's actual
// (finite) radius r0", with f0 = 1 - Rs/r0 the same Schwarzschild lapse
// already computed once per frame by the caller for the impact-parameter
// formula.
//
// sqrt(1-3M/r) is guarded against going non-positive at/inside the photon
// sphere (r <= 1.5*Rs) the same way the rest of this file guards radicands
// against float noise; (1 - s*Omega*b) is guarded by kMinDopplerDenom, and
// the whole result is clamped to kMaxDopplerFactor -- see both constants'
// documentation above for why. Physically, both guards only matter for
// rays passing extremely close to the photon sphere, where the true
// (unclamped) answer diverges to infinity anyway -- capping it trades an
// undefined/NaN pixel for a bright-but-bounded one, which is the same
// tradeoff this file already makes for RK4 winding near b_crit.
float dopplerRedshiftFactor(float r, float b, vec3 n, float M, float rotationDir, float invSqrtF0) {
    float omega = sqrt(max(M, 0.0)) / pow(max(r, 1e-6), 1.5);

    float s = sign(dot(n, vec3(0.0, 1.0, 0.0))) * rotationDir;

    float denom = 1.0 - s * omega * b;
    float denomMag = max(abs(denom), kMinDopplerDenom);
    denom = (denom < 0.0) ? -denomMag : denomMag;

    float grav = sqrt(max(1.0 - 3.0 * M / max(r, 1e-6), 1e-6));

    float g = (grav / denom) * invSqrtF0;
    return clamp(g, 0.0, kMaxDopplerFactor);
}

// Combines temperature -> color/brightness -> tonemapped emission. Flux is
// scaled as T^4 (Stefan-Boltzmann law: total blackbody radiant emittance
// scales with the fourth power of temperature), then uDiskBrightness is
// applied as a free artistic exposure multiplier on top of that physically
// motivated scaling, and the result is tonemapped with a simple exponential
// (Reinhard-family) curve so no combination of parameters can produce
// unbounded/NaN-propagating output.
// Milestone 7 parameters (b, n, M, rotationDir, invSqrtF0, relativisticEnabled)
// are additive: when relativisticEnabled == 0 (the default), T is used
// unmodified and every line below is byte-for-byte what Milestone 6
// produced, so M6's behavior is unchanged unless Milestone 7 is explicitly
// turned on.
vec3 diskEmission(float r, float innerRadius, float b, vec3 n, float M,
                   float rotationDir, float invSqrtF0, int relativisticEnabled) {
    float T = diskTemperature(r, innerRadius);

    if (relativisticEnabled != 0) {
        // T -> g*T: because a blackbody spectrum is Lorentz-invariant, a
        // redshifted/blueshifted blackbody is itself exactly a blackbody at
        // the scaled temperature g*T (see file-header comment above and
        // docs/ARCHITECTURE.md, Milestone 7). Substituting here means the
        // color ramp and the T^4 flux below automatically pick up both the
        // color shift and the relativistic beaming (g^4 intensity factor)
        // with no separate code path.
        float g = dopplerRedshiftFactor(r, b, n, M, rotationDir, invSqrtF0);
        T *= g;
    }

    float tNorm = T / max(uDiskReferenceTemperature, 1e-6);
    vec3 color = blackbodyApprox(clamp(tNorm, 0.0, 1.0));
    float flux = uDiskBrightness * pow(max(T, 0.0), 4.0);
    vec3 hdr = color * flux;
    return vec3(1.0) - exp(-hdr); // tonemap: HDR -> stable [0,1) range
}

// ---------------------------------------------------------------------
// Milestone 8, Phase 1 -- traceRay()
// ---------------------------------------------------------------------
// The complete M5 (geodesic lensing) + M6 (disk crossing) + M7
// (relativistic shading) per-ray pipeline, refactored out of main() into a
// standalone function of the ray direction D so it can be invoked once per
// pixel (the default) or multiple times per pixel with different
// subpixel-jittered directions (supersampling, see main() below) without
// duplicating any of the physics. Every line below is unchanged from the
// pre-M8 inline version in main() -- only its container and the
// FragColor writes (now `return`s of the resolved color) changed. See the
// file-header comment and docs/ARCHITECTURE.md for the physics derivation.
vec3 traceRay(vec3 D) {
    // --- Ray reconstruction and orbital-plane setup ---------------------
    vec3 C = uCameraPos;
    float M = uMass;
    float Rs = uSchwarzschildRadius;

    float r0 = length(C);

    // Degenerate: camera sits exactly at the black hole center. Not
    // physically meaningful (and not reachable in normal use -- the event
    // horizon is solid geometry in the M1-M4 path) but guarded rather than
    // dividing by r0 == 0 below.
    if (r0 < 1e-6) {
        return texture(uEnvironment, D).rgb;
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
        //
        // Milestone 6 known limitation: disk intersection is intentionally
        // not tested in this branch. A ray this close to exactly radial
        // has |D x rhat| < 1e-4, i.e. it is within a fraction of a degree
        // of pointing straight at/away from the black hole; the solid
        // angle this affects is negligible and not visually distinguishable
        // at any practical resolution, and the closed-form nature of this
        // branch (no e_r/e_phi/phi are computed here) makes the same
        // plane-crossing test used below inapplicable without reintroducing
        // the numerically unstable basis this branch exists to avoid.
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
            return vec3(0.0, 0.0, 0.0);
        }

        // Impact parameter, from the exact Schwarzschild relation.
        float b = r0 * Dphi / sqrt(f0);

        // Milestone 7: 1/sqrt(f(r0)), the finite-camera-distance correction
        // for dopplerRedshiftFactor() (see its documentation above). f0 is
        // already guaranteed > 0 by the guard above, and is the same for
        // every pixel this frame (r0 depends only on camera position), so
        // this is one extra sqrt per pixel -- cheap, and only ever read
        // when a disk crossing is actually found below.
        float invSqrtF0 = 1.0 / sqrt(f0);

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
        bool diskHit = false;
        vec3 diskColor = vec3(0.0);
        float invRs = (Rs > 0.0) ? (1.0 / Rs) : kNoHorizonInvRs; // no horizon -> never captured by radius.

        // --- Milestone 6: disk-plane crossing setup ----------------------
        // The ray's entire trajectory lies in the fixed plane spanned by
        // (e_r, e_phi) (spherically symmetric spacetime => motion is
        // planar). World-space position at any point along the path is
        // r*(cos(phi)*e_r + sin(phi)*e_phi), so its y-coordinate is
        // r*(cos(phi)*e_r.y + sin(phi)*e_phi.y) -- since r > 0 always, the
        // sign of y depends only on phi, not r. Tracking that sign across
        // RK4 steps and detecting a flip is therefore an exact (not
        // approximate) test for "the ray just crossed the world-space
        // equatorial plane y = 0", with only the crossing radius itself
        // linearly interpolated between the two straddling steps.
        // Disabled entirely (uDiskEnabled == 0) adds zero extra cost to the
        // M5 path -- the branch below is never taken.
        bool diskCheck = (uDiskEnabled != 0);
        float yPrev = r0 * e_r.y; // = C.y exactly, i.e. the camera's actual world-space height.
        float rPrev = r0;

        // --- Fixed-step RK4 integration of the orbit equation -----------
        //   du/dphi = v
        //   dv/dphi = 3*M*u^2 - u
        float uPrev = u;
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

            if (diskCheck) {
                float rCurr = 1.0 / max(u, 1e-12);
                float yCurr = rCurr * (cos(phi) * e_r.y + sin(phi) * e_phi.y);

                if (yPrev * yCurr < 0.0) {
                    // Sign flip between the previous and current step:
                    // the equatorial plane was crossed somewhere in
                    // between. Linearly interpolate the crossing radius
                    // (documented approximation -- exact within one RK4
                    // substep, i.e. within kPhiStep = 0.02 rad of travel).
                    float t = abs(yPrev) / max(abs(yPrev) + abs(yCurr), 1e-12);
                    float rCross = mix(rPrev, rCurr, t);

                    if (rCross >= uDiskInnerRadius && rCross <= uDiskOuterRadius) {
                        diskHit = true;
                        diskColor = diskEmission(rCross, uDiskInnerRadius, b, n, M,
                                                  uDiskRotationDirection, invSqrtF0,
                                                  uRelativisticEnabled);
                        break;
                    }
                    // Crossing was through the central hole (inside
                    // uDiskInnerRadius) or beyond the disk's outer edge --
                    // not a hit, keep integrating.
                }

                yPrev = yCurr;
                rPrev = rCurr;
            }

            if (u >= invRs) {
                captured = true;
                break;
            }
            if (u <= 1.0 / kEscapeRadius) {
                float uTarget = 1.0 / kEscapeRadius;
                float t = (uPrev - uTarget) / max(uPrev - u, 1e-12);
                float phiEsc = mix(phi - kPhiStep, phi, clamp(t, 0.0, 1.0));
                finalDir = cos(phiEsc) * e_r + sin(phiEsc) * e_phi;
                escaped = true;
                break;
            }

            uPrev = u;
        }

        if (diskHit) {
            return diskColor;
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

        if (!captured && !escaped) {
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
        return vec3(0.0, 0.0, 0.0);
    }

    return texture(uEnvironment, normalize(finalDir)).rgb;
}

// Milestone 8, Phase 1: cheap deterministic pseudo-random hash used only
// for subpixel jitter below -- no cryptographic property needed, just
// values that look uncorrelated from pixel to pixel and sample to sample.
// Standard fract-of-scaled-dot-products construction (Hoskins-style);
// output is in [0, 1).
float hash12(vec2 p) {
    vec3 p3 = fract(vec3(p.xyx) * 0.1031);
    p3 += dot(p3, p3.yzx + 33.33);
    return fract((p3.x + p3.y) * p3.z);
}

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

    // --- Milestone 8, Phase 1: optional NxN supersampling ---------------
    // gridSize == 1 (supersampling disabled, or an out-of-range/degenerate
    // uSupersamplingGrid) takes the single-sample path: exactly one
    // traceRay() call with the pixel-center direction already
    // reconstructed above, which is byte-for-byte the pre-M8 M5-M7
    // computation. This is the default (uSupersamplingEnabled == 0), so
    // the baseline rendered image is unchanged unless a person explicitly
    // turns supersampling on from ImGui.
    int gridSize = (uSupersamplingEnabled != 0) ? clamp(uSupersamplingGrid, 1, 4) : 1;

    if (gridSize <= 1) {
        FragColor = vec4(traceRay(worldDir), 1.0);
        return;
    }

    // Size of one screen pixel in vScreenUV's [0,1] space (see
    // lensing.vert), used to place each subsample at a fractional-pixel
    // offset from the pixel center reconstructed above.
    vec2 pixelUV = 1.0 / max(uResolution, vec2(1.0));
    float invGrid = 1.0 / float(gridSize);

    vec3 accum = vec3(0.0);

    // Stratified gridSize x gridSize subdivision of the pixel: cell
    // (sx, sy) owns the sub-square [sx, sx+1) x [sy, sy+1) * invGrid of
    // local [0,1) pixel space, so every subsample grid evenly partitions
    // the pixel regardless of jitter (no two cells' samples can clump
    // together, unlike unconstrained per-pixel random sampling).
    for (int sy = 0; sy < gridSize; ++sy) {
        for (int sx = 0; sx < gridSize; ++sx) {
            vec2 cellOrigin = vec2(float(sx), float(sy)) * invGrid;

            // Default: sample exactly at the cell's center.
            vec2 localPos = cellOrigin + vec2(0.5 * invGrid);

            if (uJitterEnabled != 0) {
                // Stratified jitter: move within this sample's own cell
                // only, so the even partition above still holds -- this is
                // what keeps stratified jitter from degenerating into
                // plain unstratified random sampling.
                vec2 seed = gl_FragCoord.xy * 13.0 + vec2(float(sx) * 7.0, float(sy) * 17.0);
                vec2 rnd = vec2(hash12(seed), hash12(seed + vec2(19.19, 7.77)));
                localPos = cellOrigin + rnd * invGrid;
            }

            // Recenter on the pixel center (localPos in [0,1) -> offset in
            // (-0.5, 0.5) pixels), then convert to a screen-UV offset and
            // reconstruct that subsample's own world-space ray direction
            // exactly the same way the pixel-center ray was reconstructed
            // above.
            vec2 subOffset = localPos - vec2(0.5);
            vec2 subUV = vScreenUV + subOffset * pixelUV;
            vec2 subNdc = subUV * 2.0 - 1.0;

            vec4 subViewSpace = uInvProjection * vec4(subNdc, -1.0, 1.0);
            subViewSpace /= subViewSpace.w;
            vec3 subDir = normalize((uInvView * vec4(subViewSpace.xyz, 0.0)).xyz);

            accum += traceRay(subDir);
        }
    }

    // Average the gridSize*gridSize subsamples.
    FragColor = vec4(accum * (invGrid * invGrid), 1.0);
}
