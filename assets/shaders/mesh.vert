#version 330 core

layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aNormal;

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProjection;

out vec3 worldPosition;
out vec3 worldNormal;

void main()
{
    vec4 position = uModel * vec4(aPosition, 1.0);
    worldPosition = position.xyz;
    worldNormal = mat3(transpose(inverse(uModel))) * aNormal;
    gl_Position = uProjection * uView * position;
}
