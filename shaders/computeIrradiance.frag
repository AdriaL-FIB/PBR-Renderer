#version 330

in vec3 dir;

uniform samplerCube specular_map;

out vec4 frag_color;

const float PI = 3.14159265359;

void main (void) {

    float step = 0.02;

    vec3 normal = normalize(dir);

    vec3 up = vec3(0.f, 1.f, 0.f);
    vec3 right = normalize(cross(up, normal));
    up = normalize(cross(normal, right));

    vec3 irradiance = vec3(0.f);
    float totalSamples = 0.f; 
    for (float phi = 0.f; phi < 2.f * PI; phi += step) {
        for (float theta = 0.f; theta < 0.5f * PI; theta += step) {
            vec3 tangentCoords = vec3(sin(theta) * cos(phi),  sin(theta) * sin(phi), cos(theta));
            vec3 worldCoords = tangentCoords.x * right + tangentCoords.y * up + tangentCoords.z * normal; 
            irradiance += texture(specular_map, worldCoords).rgb * cos(theta) * sin(theta);
            totalSamples++;
        }
    }
    irradiance = PI * irradiance * (1.f / float(totalSamples));

    frag_color = vec4(irradiance, 1.f);
}
