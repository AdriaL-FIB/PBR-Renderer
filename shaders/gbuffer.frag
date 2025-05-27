#version 330

//#define USE_TEXTURES

in vec3 normal_eye;
in vec3 pos_eye;

uniform sampler2D current_texture;

in vec2 uvs;

// color attachments
layout(location = 0) out vec4 gAlbedo;
layout(location = 1) out vec3 gNormal;


vec4 red = vec4(1.f, 0.f, 0.f, 1.f);
vec4 white = vec4(1.f, 1.f, 1.f, 1.f);
vec4 blue = vec4(0.f, 0.f, 1.f, 1.f);

void main (void) {
    gAlbedo = blue;
#ifdef USE_TEXTURES
    gAlbedo = texture(current_texture, uvs);
#endif
    gNormal = normalize(normal_eye);
}
