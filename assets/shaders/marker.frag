#version 330 core

out vec4 fragmentColor;

uniform vec3 uColor;

void main()
{
    vec2 centered = gl_PointCoord * 2.0 - 1.0;
    float radiusSquared = dot(centered, centered);
    if (radiusSquared > 1.0) {
        discard;
    }

    vec3 borderColor = uColor * 0.16;
    float inside = 1.0 - smoothstep(0.52, 0.70, radiusSquared);
    fragmentColor = vec4(mix(borderColor, uColor, inside), 1.0);
}
