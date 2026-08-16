#version 330 core

in vec3 worldPosition;
in vec3 worldNormal;

uniform vec3 uBaseColor;
uniform vec3 uLightPosition;
uniform vec3 uCameraPosition;
uniform float uShininess;
uniform int uRenderMode;

out vec4 fragmentColor;

void main()
{
    vec3 normal = normalize(worldNormal);
    if (uRenderMode == 2) {
        fragmentColor = vec4(normal * 0.5 + 0.5, 1.0);
        return;
    }

    vec3 lightDirection = normalize(uLightPosition - worldPosition);
    vec3 viewDirection = normalize(uCameraPosition - worldPosition);
    vec3 halfwayDirection = normalize(lightDirection + viewDirection);

    float diffuse = max(dot(normal, lightDirection), 0.0);
    float specular = pow(max(dot(normal, halfwayDirection), 0.0), uShininess);
    float rim = pow(1.0 - max(dot(normal, viewDirection), 0.0), 2.0);

    vec3 color = uBaseColor * (0.20 + 0.80 * diffuse);
    color += vec3(0.42) * specular;
    color += vec3(0.08, 0.16, 0.18) * rim;
    fragmentColor = vec4(color, 1.0);
}
