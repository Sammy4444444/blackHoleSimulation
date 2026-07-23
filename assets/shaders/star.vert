#version 330 core

layout(location = 0) in vec3 aPosition;

uniform mat4 uView;
uniform mat4 uProjection;

out float vHash;

// Cheap deterministic hash of a star's world-space position, used for
// per-star brightness/color/size variation. Same value is derivable in the
// fragment shader from the interpolated hash below, but computing it once
// per-vertex (each point is a single vertex) is enough since there is no
// interpolation across a point sprite's "face" beyond gl_PointCoord.
float hash(vec3 p) {
    return fract(sin(dot(p, vec3(12.9898, 78.233, 45.164))) * 43758.5453);
}

void main() {
    gl_Position = uProjection * uView * vec4(aPosition, 1.0);

    vHash = hash(aPosition);

    // Vary apparent size per star (small, twinkly dots rather than uniform
    // blobs), biased toward the smaller end.
    gl_PointSize = 1.0 + vHash * 2.0;
}
