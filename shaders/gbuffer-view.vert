#version 330

layout (location = 0) in vec2 vert;
layout (location = 1) in vec2 texCoord;

out vec2 uvs;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

uniform mat3 normal_matrix;

void main(void)  {
    uvs = texCoord;
    gl_Position = vec4(vert, 0, 1);
}
