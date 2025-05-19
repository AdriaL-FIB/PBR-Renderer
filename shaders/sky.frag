#version 330

in vec3 dir;

uniform samplerCube specular_map;

out vec4 frag_color;

void main (void) {
    frag_color = texture(specular_map, dir);
    //frag_color = vec4(1.f,1.f,0.f,1.f);
}
