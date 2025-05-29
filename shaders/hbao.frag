#version 330


in vec2 uvs;

out vec4 frag_color;

uniform float zNear;
uniform float zFar;
uniform int numDirs;
uniform int nSamples;
uniform float searchRadius;

const float PI = 3.14159265359;

float LinearizeDepth(float depth) {
    float z = depth * 2.0 - 1.0; // de [0,1] a NDC [-1,1]
    return (2.0 * zNear * zFar) / (zFar + zNear - z * (zFar - zNear));
}

void main (void) {
    float step_ = searchRadius / nSamples; 
    for (int dir = 0; dir < numDirs; ++dir) {
        float angle = (dir / numDirs) * 2 * PI;
        for (int smpl = 0; smpl < nSamples; ++smpl) {
            
        }
    }
}
