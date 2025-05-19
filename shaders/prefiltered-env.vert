#version 330

layout (location = 0) in vec3 vert;

uniform mat4 view;
uniform mat4 projection;

uniform mat3 normal_matrix;

out vec3 dir;

void main(void)  {
    // dir = (projection * view * vec4(vert, 1.f)).xyz;
    dir = vert;
    //normal_es = normalize(normal_matrix * normal);
    gl_Position = (projection * view * vec4(vert, 1.f)).xyww;
}
