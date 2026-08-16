#version 330 core

uniform vec3 uPosition;
uniform mat4 uView;
uniform mat4 uProjection;

void main()
{
    gl_Position = uProjection * uView * vec4(uPosition, 1.0);
    gl_PointSize = 15.0;
}
