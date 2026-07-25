#version 330 core

// Milestone 5 — fullscreen-triangle scaffold (Phase 1).
//
// No vertex buffer is bound for this draw call. A single oversized triangle
// covering the whole screen is generated purely from gl_VertexID, which is
// the standard OpenGL 3.3-core-legal way to do a fullscreen pass without a
// VBO/EBO: 3 vertices, drawn with glDrawArrays(GL_TRIANGLES, 0, 3) against a
// bound-but-empty VAO. This deliberately does not reuse Mesh — Mesh assumes
// CPU-side vertex data to upload, which does not apply here.
//
// Phase 5 will add camera-ray reconstruction in this shader (or pass the
// necessary uniforms through untouched from here); Phase 1 only establishes
// the triangle and the NDC/UV plumbing.

out vec2 vScreenUV;

void main() {
    // Encodes a triangle whose three corners lie outside [-1, 1] on at least
    // one axis, so that after the rasterizer clips it to the viewport, the
    // visible portion exactly covers the screen with no seam (unlike a
    // 2-triangle quad, which has a diagonal seam and 4 vertices instead of 3).
    vec2 ndc = vec2(
        (gl_VertexID == 2) ?  3.0 : -1.0,
        (gl_VertexID == 1) ? -3.0 :  1.0
    );

    gl_Position = vec4(ndc, 0.0, 1.0);

    // UV in [0, 1] across the visible screen, for use once ray reconstruction
    // is added in a later phase.
    vScreenUV = ndc * 0.5 + 0.5;
}
