#version 330 core

in float vHash;

out vec4 FragColor;

void main() {
    // Circular soft falloff instead of a square dot: gl_PointCoord is in
    // [0, 1] across the sprite, so recenter to [-0.5, 0.5] and use distance
    // from the center to fade alpha toward the edge.
    vec2 centered = gl_PointCoord - vec2(0.5);
    float dist = length(centered);
    float falloff = 1.0 - smoothstep(0.0, 0.5, dist);
    if (falloff <= 0.0) {
        discard;
    }

    // Brightness and a slight color-temperature tint derived from the same
    // per-star hash used for point size in the vertex shader, so a given
    // star's size, brightness, and tint are all consistently correlated.
    float brightness = 0.6 + vHash * 0.4;
    vec3 warm = vec3(1.0, 0.85, 0.7);
    vec3 cool = vec3(0.75, 0.85, 1.0);
    vec3 tint = mix(warm, cool, vHash);

    FragColor = vec4(tint * brightness, falloff);
}
