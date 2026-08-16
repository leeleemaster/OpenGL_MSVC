#version 330 core

uniform vec3 uColor;

out vec4 fragmentColor;

void main()
{
    fragmentColor = vec4(uColor, 1.0);
}
