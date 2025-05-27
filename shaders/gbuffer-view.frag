#version 330

in vec2 uvs;
out vec4 frag_color;

uniform sampler2D gAlbedoSampler;
uniform sampler2D gNormalSampler;
uniform sampler2D gDepthSampler;


uniform int current_texture;
uniform float zNear;
uniform float zFar;

float LinearizeDepth(float depth) {
    float z = depth * 2.0 - 1.0; // de [0,1] a NDC [-1,1]
    return (2.0 * zNear * zFar) / (zFar + zNear - z * (zFar - zNear));
}

void main (void) {
    if (current_texture == 0) { // Albedo
        frag_color = texture(gAlbedoSampler, uvs);
    } else if (current_texture == 1) { // Normales
        vec3 normal = texture(gNormalSampler, uvs).rgb;
        frag_color = vec4(normal * 0.5 + 0.5, 1.0); // Mapear de [-1,1] a [0,1]
    } else if (current_texture == 2) { // Profundidad
        float depthValue = texture(gDepthSampler, uvs).r; // ¡Importante: .r para profundidad!

        // Opcional: Visualizar profundidad linealizada y normalizada
        // float linearDepth = LinearizeDepth(depthValue);
        // float normalizedLinearDepth = (linearDepth - u_zNear) / (u_zFar - u_zNear);
        // FragColor = vec4(vec3(normalizedLinearDepth), 1.0);

        // Visualizar profundidad cruda (generalmente se ve casi todo blanco o negro)
        frag_color = vec4(vec3(depthValue), 1.0);
    } else {
        frag_color = vec4(1.0, 0.0, 1.0, 1.0); // Color de error (magenta)
    }
}
