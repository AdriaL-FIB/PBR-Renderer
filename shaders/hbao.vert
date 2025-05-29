#version 330

layout (location = 0) in vec3 vert;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec2 texCoord;

out vec3 normal_eye;
out vec2 uvs;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

uniform mat3 normal_matrix;

void main(void)  {
    uvs = texCoord;
    normal_eye = normalize(normal_matrix * normal);
    gl_Position = projection * view * model * vec4(vert, 1.0);
}
