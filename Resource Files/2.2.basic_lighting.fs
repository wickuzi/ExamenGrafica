#version 330 core
out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoord;

uniform vec3 objectColor;
uniform float objectAlpha;
uniform vec3 viewPos;
uniform sampler2D texture1;
uniform float alphaCutoff;
uniform float emissiveStrength;

// Silent Hill style fog and grade
uniform vec3 fogColor;
uniform float fogDensity;
uniform float fogStart;
uniform float fogEnd;
uniform int fogEnabled;

// material
uniform float material_shininess;
uniform float material_specularStrength;
uniform float material_ambientStrength;

// directional light
uniform vec3 dirLight_direction;
uniform vec3 dirLight_color;

// point light
// multiple point lights
#define MAX_POINT_LIGHTS 16
struct PointLight {
    vec3 position;
    vec3 color;
    float constant;
    float linear;
    float quadratic;
};
uniform int numPointLights;
uniform PointLight pointLights[MAX_POINT_LIGHTS];

// spotlight
uniform vec3 spotLight_position;
uniform vec3 spotLight_direction;
uniform vec3 spotLight_color;
uniform float spotLight_cutOff; // cos(inner)
uniform float spotLight_outerCutOff; // cos(outer)

// calculate directional light contribution
vec3 CalcDirectionalLight(vec3 normal, vec3 viewDir)
{
    vec3 lightDir = normalize(-dirLight_direction);
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), material_shininess);
    vec3 ambient = material_ambientStrength * dirLight_color;
    vec3 diffuse = diff * dirLight_color;
    vec3 specular = material_specularStrength * spec * dirLight_color;
    return ambient + diffuse + specular;
}

// calculate point light contribution with attenuation (for one PointLight)
vec3 CalcPointLight(PointLight pl, vec3 normal, vec3 fragPos, vec3 viewDir)
{
    vec3 lightDir = normalize(pl.position - fragPos);
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), material_shininess);
    float distance = length(pl.position - fragPos);
    float attenuation = 1.0 / (pl.constant + pl.linear * distance + pl.quadratic * (distance * distance));
    vec3 ambient = material_ambientStrength * pl.color * attenuation;
    vec3 diffuse = diff * pl.color * attenuation;
    vec3 specular = material_specularStrength * spec * pl.color * attenuation;
    return ambient + diffuse + specular;
}

// calculate spotlight contribution with soft edges
vec3 CalcSpotLight(vec3 normal, vec3 fragPos, vec3 viewDir)
{
    vec3 lightDir = normalize(spotLight_position - fragPos);
    float theta = dot(lightDir, normalize(-spotLight_direction));
    float epsilon = spotLight_cutOff - spotLight_outerCutOff;
    float intensity = clamp((theta - spotLight_outerCutOff) / epsilon, 0.0, 1.0);
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), material_shininess);
    float distance = length(spotLight_position - fragPos);
    float attenuation = 1.0 / (1.0 + 0.09 * distance + 0.032 * distance * distance);
    vec3 ambient = material_ambientStrength * spotLight_color * attenuation * intensity;
    vec3 diffuse = diff * spotLight_color * attenuation * intensity;
    vec3 specular = material_specularStrength * spec * spotLight_color * attenuation * intensity;
    return ambient + diffuse + specular;
}

void main()
{
    vec3 norm = normalize(Normal);
    vec3 viewDir = normalize(viewPos - FragPos);

    vec3 resultLighting = vec3(0.0);
    resultLighting += CalcDirectionalLight(norm, viewDir);
    for (int i = 0; i < numPointLights; ++i)
        resultLighting += CalcPointLight(pointLights[i], norm, FragPos, viewDir);

    vec4 texSample = texture(texture1, TexCoord);
    float finalAlpha = texSample.a * objectAlpha;
    if (finalAlpha < alphaCutoff)
        discard;

    vec3 baseColor = texSample.rgb * objectColor;
    vec3 result = resultLighting * baseColor;
    result += baseColor * emissiveStrength;

    // Light desaturation keeps the Silent Hill mood without hiding the map textures.
    float gray = dot(result, vec3(0.299, 0.587, 0.114));
    result = mix(vec3(gray), result, 0.72);
    result *= vec3(1.02, 1.02, 0.94);

    if (fogEnabled == 1) {
        float distanceToCamera = length(viewPos - FragPos);
        float linearFog = clamp((fogEnd - distanceToCamera) / (fogEnd - fogStart), 0.0, 1.0);
        float expFog = exp(-fogDensity * distanceToCamera);
        float fogFactor = clamp(min(linearFog, expFog), 0.0, 1.0);
        result = mix(fogColor, result, fogFactor);
    }

    FragColor = vec4(result, 1.0);
}
