#version 330

in vec2 uvs;

out vec4 frag_color;

uniform float zNear;
uniform float zFar;
uniform int numDirs;
uniform int nSamples;
uniform float searchRadius;

uniform float fov;
uniform float aspectRatio;

uniform sampler2D depthTex;
uniform sampler2D normalsTex;

uniform bool improvements;

const float PI = 3.14159265359;

float height = 2 * zNear * tan(fov/2);
float width = aspectRatio * height;

float rand(vec2 co) {
    return fract(sin(dot(co.xy ,vec2(12.9898,78.233))) * 43758.5453);
}

float linearizeDepth(float depth) {
    float z_ndc = depth * 2.f - 1.f;
    return (2.f * zNear * zFar) / (zFar + zNear - z_ndc * (zFar - zNear));
}

vec3 recoverEyePos(vec2 tex_coords) {
    float z_eye = linearizeDepth(texture(depthTex, tex_coords).r);

    float x_at_znear = (tex_coords.x * 2.f - 1.f) * width/2.f;
    float y_at_znear = (tex_coords.y * 2.f - 1.f) * height/2.f;

    float x_eye = x_at_znear * z_eye / zNear;
    float y_eye = y_at_znear * z_eye / zNear;
    return vec3(x_eye, y_eye, z_eye);
}

const float bias = 0.05f;

void main (void) {
    vec3 P = recoverEyePos(uvs);
    vec3 n = texture(normalsTex, uvs).rgb;

    float ao = 0;

    float searchRadius_uv = searchRadius / (2 * P.z * tan(fov/2.f));

    float init_phase = 0;
    if (improvements) {
        init_phase = rand(uvs);
    }

    float step_ = searchRadius_uv / nSamples; 
    for (int d = 0; d < numDirs; ++d) {
        float angle = (d / float(numDirs)) * 2 * PI + init_phase;
        vec2 dir = vec2(cos(angle), sin(angle));

        float t_theta = atan(-dot(dir,n.xy),n.z);

        float h_theta = t_theta;
        float r_theta = 0;
        for (int i = 1; i <= nSamples; ++i) {
            vec2 S_uv = uvs + dir * step_ * i;
            if (S_uv.x < 0.0 || S_uv.x > 1.0 || S_uv.y < 0.0 || S_uv.y > 1.0) {
                continue;
            }
            vec3 S = recoverEyePos(S_uv);

            vec3 D = S - P;
            float dist_xy = length(D.xy);
            float dist_d = length(D);

            if (dist_d > searchRadius)
                continue;

            float alpha = atan(-D.z, dist_xy);

            if (alpha > h_theta) {
                h_theta = alpha;
                r_theta = dist_d;
            }
        }

        ao += (sin(h_theta) - sin(t_theta + bias * int(improvements))) * (max(0, 1 - r_theta / searchRadius));
    }

    ao /= float(numDirs);

    frag_color = vec4(vec3(1.f - ao), 1.f);
    // frag_color = vec4(1.f, 0.f, 1.f, 1.f);
}