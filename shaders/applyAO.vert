#version 330

layout (location = 0) in vec3 vert;
layout (location = 1) in vec2 texCoord;

out vec2 uvs;

void main(void) {
    uvs = texCoord;
    gl_Position = vec4(vert, 1.0);
}
