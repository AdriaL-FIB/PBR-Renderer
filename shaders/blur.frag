#version 330 core

out vec4 frag_color;
in vec2 uvs;

uniform sampler2D tex;
uniform int width;
uniform int height;
uniform bool dir;

const float w[5] = float[] (0.06136f, 0.24477f, 0.38774f, 0.24477f, 0.06136f);

void main()
{

    float texelSize = dir ? 1.f / float(width) : 1.f / float(height);

    vec3 result = vec3(0.f);
    for (int i = 0; i < 5; i++) {
        float offset = (float(i) - 2.f) * texelSize;
        vec2 increment = dir ? vec2(offset, 0.f) : vec2(0.f, offset);
        result += texture(tex, uvs + increment).rgb * w[i];
    }

    frag_color = vec4(result, 1.f);
}
