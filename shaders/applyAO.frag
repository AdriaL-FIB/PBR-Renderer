#version 330 core

out vec4 frag_color;
in vec2 uvs;

uniform sampler2D albedoTex;
uniform sampler2D aoTex;


void main() {
    vec3 color = texture(albedoTex, uvs).rgb;
    vec3 ao    = texture(aoTex    , uvs).rgb;
    frag_color = vec4(color * ao, 1.f);
}
