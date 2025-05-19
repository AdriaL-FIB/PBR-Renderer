#version 330

in vec3 normal_world;
in vec3 pos_world;

out vec4 frag_color;

//uniform vec3 camera_pos;

uniform samplerCube specular_map;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;


void main (void) {

    vec3 camera_pos = (inverse(view) * vec4(0,0,0,1)).xyz;

    vec3 N = normalize(normal_world);

    vec3 V = normalize(camera_pos - pos_world);

    vec3 R = reflect(-V, N);

    frag_color = texture(specular_map, R);
}
