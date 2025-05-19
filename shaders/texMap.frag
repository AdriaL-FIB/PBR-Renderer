#version 330

out vec4 frag_color;

uniform sampler2D current_texture;

in vec2 uvs;

void main (void) {
    frag_color = texture(current_texture, uvs);
}
