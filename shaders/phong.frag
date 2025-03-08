#version 330 core

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;

out vec4 FragColor;

uniform vec3 viewPos;
uniform vec3 lightPos;
uniform vec3 lightColor;
uniform vec3 objectColor;

// Material properties
uniform float ambientStrength;
uniform float diffuseStrength;
uniform float specularStrength;
uniform float shininess;

// Texture samplers
uniform sampler2D texture_diffuse1;
uniform sampler2D texture_specular1;
uniform bool hasTexture;

// Constants
const float gamma = 2.2;

vec3 sRGBToLinear(vec3 color) {
    return pow(color, vec3(gamma));
}

vec3 linearToSRGB(vec3 color) {
    return pow(color, vec3(1.0/gamma));
}

void main()
{
    // Get base color from texture or material
    vec3 baseColor;
    if (hasTexture) {
        vec4 texColor = texture(texture_diffuse1, TexCoords);
        if (texColor.a < 0.1) discard; // Discard fully transparent pixels
        baseColor = texColor.rgb;
    } else {
        baseColor = texture(texture_diffuse1, vec2(0.0)).rgb; // Use material color stored in texture
    }

    // Normalize vectors for lighting calculations
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos);
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 halfwayDir = normalize(lightDir + viewDir);

    // Ambient lighting
    vec3 ambient = ambientStrength * lightColor * baseColor;

    // Diffuse lighting
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * lightColor * baseColor;

    // Specular lighting
    float spec = pow(max(dot(norm, halfwayDir), 0.0), shininess);
    vec3 specular = specularStrength * spec * lightColor;

    // Combine lighting components
    vec3 result = ambient + diffuse + specular;

    // Tone mapping and gamma correction
    result = result / (result + vec3(1.0)); // Reinhard tone mapping
    result = pow(result, vec3(1.0/2.2)); // Gamma correction

    FragColor = vec4(result, 1.0);
} 