#version 330 core

out vec4 fragmentColor;

void main()
{
    vec2 centered = gl_PointCoord * 2.0 - 1.0;
    float radiusSquared = dot(centered, centered);
    if (radiusSquared > 1.0) {
        discard;
    }

    vec3 borderColor = vec3(0.12, 0.025, 0.01);
    vec3 markerColor = vec3(1.0, 0.30, 0.055);
    float inside = 1.0 - smoothstep(0.52, 0.70, radiusSquared);
    fragmentColor = vec4(mix(borderColor, markerColor, inside), 1.0);
}
