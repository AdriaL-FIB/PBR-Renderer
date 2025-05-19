#version 330

in vec3 normal_eye;
in vec3 pos_eye;

out vec4 frag_color;

uniform vec3 light; // world space

uniform mat4 view;
uniform mat4 projection;

vec4 red = vec4(1.f, 0.f, 0.f, 1.f);
vec4 white = vec4(1.f, 1.f, 1.f, 1.f);
vec4 blue = vec4(0.f, 0.f, 1.f, 1.f);

void main (void) {

    vec4 ka = vec4((0.2 * white).xyz, 1.f);
    vec4 kd = vec4((0.6 * white).xyz, 1.f);
    vec4 ks = vec4((0.8 * white).xyz, 1.f);

    vec4 ia = white;
    vec4 id = white;
    vec4 is = white;
    
    vec3 N = normalize(normal_eye);

    vec3 light_eye = (view * vec4(light, 1.f)).xyz;
    vec3 L = normalize(light_eye - pos_eye);


    vec3 R = normalize(2 * dot(L, N) * N - L);
    vec3 V = normalize(-pos_eye);

    vec4 ambient_component = ka * ia;
    vec4 diffuse_component = kd * max(dot(L, N), 0.f) * id;
    vec4 specular_component = ks * pow(max(dot(R, V), 0.f), 30) * is;

    frag_color = ambient_component + specular_component + diffuse_component;
    
}
