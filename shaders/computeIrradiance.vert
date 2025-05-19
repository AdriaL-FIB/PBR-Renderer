#version 330

layout (location = 0) in vec3 vert;

uniform mat4 view;
uniform mat4 projection;

out vec3 dir;

void main(void)  {
    dir = vert;
    gl_Position = (projection * view * vec4(vert, 1.f)).xyww;
}
