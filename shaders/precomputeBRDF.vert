#version 330

layout (location = 0) in vec2 vert;
layout (location = 1) in vec2 texCoord;


out vec2 uvs;

void main(void)  {
    // dir = (projection * view * vec4(vert, 1.f)).xyz;
    uvs = texCoord;
    //normal_es = normalize(normal_matrix * normal);
    gl_Position = vec4(vert, 0, 1);
}
