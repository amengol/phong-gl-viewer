#version 330 core

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;

out vec4 FragColor;

uniform vec3 viewPos;
uniform vec3 lightPos;
uniform vec3 lightColor;
uniform vec3 objectColor;  // This will be our material's diffuse color

// Material properties
uniform float ambientStrength;
uniform float diffuseStrength;
uniform float specularStrength;
uniform float shininess;

// Texture samplers
uniform sampler2D texture_diffuse1;
uniform bool hasTexture;

// Constants for gamma correction
const float gamma = 2.2;
const float invGamma = 1.0 / gamma;

// Convert from sRGB to linear space
vec3 srgbToLinear(vec3 srgb) {
    return pow(srgb, vec3(gamma));
}

// Convert from linear to sRGB space
vec3 linearToSRGB(vec3 linear) {
    return pow(linear, vec3(invGamma));
}

void main()
{
    // Get base color (either from texture or uniform)
    vec3 baseColor = hasTexture ? texture(texture_diffuse1, TexCoords).rgb : objectColor;
    
    // Normalize vectors
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos);
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 halfwayDir = normalize(lightDir + viewDir);
    
    // Ambient
    vec3 ambient = ambientStrength * baseColor;
    
    // Diffuse
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diffuseStrength * diff * baseColor;
    
    // Specular (using Blinn-Phong)
    float spec = pow(max(dot(norm, halfwayDir), 0.0), shininess);
    vec3 specular = specularStrength * spec * lightColor;
    
    // Combine components
    vec3 result = (ambient + diffuse + specular) * lightColor;
    
    // Basic tone mapping
    result = result / (result + vec3(1.0));
    
    FragColor = vec4(result, 1.0);
} 