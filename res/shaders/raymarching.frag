#version 460

layout(location = 0) out vec4 outColor;

layout(std430, set = 0, binding = 0) readonly buffer SphereBuffer {
    vec4 spheres[]; // xyz -> center, w -> radius
};

layout(push_constant) uniform PushConstants {
    vec4  player;           
    vec2  resolution;
    float boundingRadius;
    uint  sphereCount;
} pc;

float sdSphere(vec3 p, vec4 s) {
    return length(p - s.xyz) - s.w;
}

float sceneSDF(vec3 p) {
    float d = 1e20;
    for (uint i = 0; i < pc.sphereCount; i++)
        d = min(d, sdSphere(p, spheres[i]));
    return d;
}

// Surface normal = gradient of the SDF (central differences)
vec3 calcNormal(vec3 p) {
    const vec2 e = vec2(0.001, 0.0);
    return normalize(vec3(
        sceneSDF(p + e.xyy) - sceneSDF(p - e.xyy),
        sceneSDF(p + e.yxy) - sceneSDF(p - e.yxy),
        sceneSDF(p + e.yyx) - sceneSDF(p - e.yyx)));
}

void main() {
    vec2 uv = (gl_FragCoord.xy - 0.5 * pc.resolution) / pc.resolution.y;
    uv.y = -uv.y; // Vulkan's framebuffer y axis points down

    vec3 ro = pc.player.xyz;
    vec3 rd = normalize(vec3(uv, 0.5 / tan(pc.player.w * 0.5)));

    float t = 0.0;
    for (int step = 0; step < 128 && t < pc.boundingRadius; step++) {
        vec3 p = ro + rd * t;
        float d = sceneSDF(p);
        if (d < 0.001) {
            vec3 n = calcNormal(p);
            vec3 lightDir = normalize(vec3(0.6, 0.8, -0.4));
            float diffuse = max(dot(n, lightDir), 0.0);
            vec3 color = vec3(0.9, 0.5, 0.3) * (0.15 + 0.85 * diffuse);
            outColor = vec4(color, 1.0);
            return;
        }
        t += d;
    }
    outColor = vec4(0.03, 0.03, 0.05, 1.0);
}
