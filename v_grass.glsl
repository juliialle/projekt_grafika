#version 330 core

layout(location = 0) in vec4 vertex;
layout(location = 1) in vec2 texCoord;

uniform mat4 M, V, P;

out vec3 worldPos;
out vec2 TexCoord;

void main() {
    vec4 pos = M * vertex;
    worldPos = pos.xyz;
    TexCoord = texCoord;
    gl_Position = P * V * pos;
}
