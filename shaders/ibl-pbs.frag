#version 330

#define USE_TEXTURES

in vec3 normal_eye;
in vec3 pos_eye;
in vec3 normal_world;
in vec3 pos_world;
in vec2 uvs;

out vec4 frag_color;

uniform samplerCube specular_map;
uniform samplerCube diffuse_map;
uniform samplerCube prefilter_map;
uniform sampler2D   brdfLUT;

uniform sampler2D color_map;
uniform sampler2D roughness_map;
uniform sampler2D metalness_map;



uniform mat4 view;

uniform vec3 fresnel;

uniform float roughness;
uniform float metalness;

vec4 red = vec4(1.f, 0.f, 0.f, 1.f);
vec4 white = vec4(1.f, 1.f, 1.f, 1.f);
// vec4 blue = vec4(0.f, 0.f, 1.f, 1.f);

const float PI = 3.14159265359;



float D(vec3 h, float a, vec3 N) {

    //GXX
    float NdotH = max(dot(N, h), 0.0);
    float denom = (NdotH * NdotH) * (a*a - 1.0) + 1.0;
    return (a*a) / (PI * denom * denom);

}

vec3 F_Schlick(vec3 F0, float cosTheta) {
    vec3 F = F0 + (1.f - F0) * pow( 1.f - cosTheta , 5);
    return F;
}

float G_SchlickGGX(float NdotV, float k) {
    //float NdotV = max(dot(n, v), 0);
    return NdotV / (NdotV * (1 - k) + k);
}


float G_Smith(float NdotV, float NdotL, float k) {
    return G_SchlickGGX(NdotV, k) * G_SchlickGGX(NdotL, k);
}

void main (void) {

    vec3 camera_pos = (inverse(view) * vec4(0,0,0,1)).xyz;

    vec3 N = normalize(normal_world);
    vec3 V = normalize(camera_pos - pos_world);
    vec3 L = V;
    vec3 h = normalize(L + V); // = V = L

    vec3 R = reflect(-V, N);   

    float NdotL = max(dot(N, L), 0);
    float NdotV = max(dot(N, V), 0);

    vec3 albedo = white.xyz;

#ifdef USE_TEXTURES
    float roughness = texture(roughness_map, uvs).r;
    float metalness = texture(metalness_map, uvs).r;
    albedo = texture(color_map, uvs).rgb;
#endif

    // float a = roughness;
    float a = roughness;
    float k = (a*a)/2;

    // vec3 R = reflect(-V, N);
    // vec3 albedo = texture(specular_map, R).xyz;

    vec3 F0 = mix(fresnel, albedo, metalness);

    float cosTheta = max(dot(V, N), 0);
    vec3 F = F_Schlick(F0, cosTheta);
    float G = G_Smith(NdotV, NdotL, k);
    float D = D(h, a, N);

    // vec3 specular = F * G * D / (4 * max(dot(N, L), 0) * max(dot(N, V), 0));

    vec3 kS = F;
    vec3 kD = (1 - kS) * (1 - metalness);

    vec3 irradiance = texture(diffuse_map, N).rgb;
    vec3 diffuse = irradiance * albedo;

    const float max_lod = 4.0;
    vec3 prefilteredColor = textureLod(prefilter_map, R, roughness * max_lod).rgb;

    vec2 envBRDF = texture(brdfLUT, vec2(max(dot(N, V), 0.0), roughness)).rg;
    vec3 specular = prefilteredColor * (F * envBRDF.x + envBRDF.y);

    vec3 result = kD * diffuse + specular;

    // gamma correction
    vec3 color = pow(result, vec3(1.0/2.2));
    
    frag_color = vec4(color, 1.0);
}
