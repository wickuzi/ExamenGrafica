#version 330 core

out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoord;

struct PointLight
{
    vec3 position;
    vec3 color;
    float constant;
    float linear;
    float quadratic;
};

const int MAX_POINT_LIGHTS = 32;

uniform sampler2D texture1;
uniform vec3 objectColor;
uniform float objectAlpha;
uniform int useTextureAlpha;
uniform int useWhiteChromaKey;
uniform int foliageMaterial;
uniform float alphaCutoff;

uniform vec3 viewPos;
uniform vec3 dirLight_direction;
uniform vec3 dirLight_color;
uniform int numPointLights;
uniform PointLight pointLights[MAX_POINT_LIGHTS];

uniform int spotLight_enabled;
uniform vec3 spotLight_position;
uniform vec3 spotLight_direction;
uniform vec3 spotLight_color;
uniform float spotLight_cutOff;
uniform float spotLight_outerCutOff;

uniform float material_shininess;
uniform float material_specularStrength;
uniform float material_ambientStrength;
uniform float emissiveStrength;

uniform int fogEnabled;
uniform vec3 fogColor;
uniform float fogStart;
uniform float fogEnd;
uniform float fogDensity;
uniform float fogTime;
uniform vec3 fogCenter;

vec3 calculateDirectionalLight(vec3 normal, vec3 viewDir, vec3 baseColor)
{
    vec3 lightDir = normalize(-dirLight_direction);
    float diffuseStrength = max(dot(normal, lightDir), 0.0);
    vec3 reflectDir = reflect(-lightDir, normal);
    float specular = pow(max(dot(viewDir, reflectDir), 0.0), max(material_shininess, 1.0));

    vec3 ambient = material_ambientStrength * dirLight_color * baseColor;
    vec3 diffuse = diffuseStrength * dirLight_color * baseColor;
    vec3 specularColor = specular * material_specularStrength * dirLight_color;
    return ambient + diffuse + specularColor;
}

vec3 calculatePointLight(PointLight light, vec3 normal, vec3 viewDir, vec3 baseColor)
{
    vec3 lightDir = normalize(light.position - FragPos);
    float diffuseStrength = max(dot(normal, lightDir), 0.0);
    vec3 reflectDir = reflect(-lightDir, normal);
    float specular = pow(max(dot(viewDir, reflectDir), 0.0), max(material_shininess, 1.0));

    float distanceToLight = length(light.position - FragPos);
    float attenuation = 1.0 / max(light.constant + light.linear * distanceToLight +
                                  light.quadratic * distanceToLight * distanceToLight, 0.001);

    vec3 diffuse = diffuseStrength * light.color * baseColor;
    vec3 specularColor = specular * material_specularStrength * light.color;
    return (diffuse + specularColor) * attenuation;
}

vec3 calculateSpotLight(vec3 normal, vec3 viewDir, vec3 baseColor)
{
    vec3 lightDir = normalize(spotLight_position - FragPos);
    float theta = dot(lightDir, normalize(-spotLight_direction));
    float epsilon = max(spotLight_cutOff - spotLight_outerCutOff, 0.001);
    float intensity = clamp((theta - spotLight_outerCutOff) / epsilon, 0.0, 1.0);
    if (intensity <= 0.0)
        return vec3(0.0);

    float diffuseStrength = max(dot(normal, lightDir), 0.0);
    vec3 reflectDir = reflect(-lightDir, normal);
    float specular = pow(max(dot(viewDir, reflectDir), 0.0), max(material_shininess, 1.0));
    float distanceToLight = length(spotLight_position - FragPos);
    float attenuation = 1.0 / (1.0 + 0.08 * distanceToLight + 0.018 * distanceToLight * distanceToLight);

    vec3 diffuse = diffuseStrength * spotLight_color * baseColor;
    vec3 specularColor = specular * material_specularStrength * spotLight_color;
    return (diffuse + specularColor) * attenuation * intensity;
}

float fogNoise(vec3 p)
{
    return fract(sin(dot(p, vec3(12.9898, 78.233, 37.719))) * 43758.5453);
}

void main()
{
    vec4 sampled = texture(texture1, TexCoord);
    vec3 baseColor = sampled.rgb * objectColor;
    float alpha = objectAlpha;
    if (useTextureAlpha == 1)
        alpha *= sampled.a;

    if (useWhiteChromaKey == 1 && sampled.r > 0.94 && sampled.g > 0.94 && sampled.b > 0.94)
        discard;
    if (alpha < alphaCutoff)
        discard;

    vec3 normal = normalize(Normal);
    if (!gl_FrontFacing)
        normal = -normal;
    vec3 viewDir = normalize(viewPos - FragPos);

    vec3 color = calculateDirectionalLight(normal, viewDir, baseColor);
    int lightCount = min(numPointLights, MAX_POINT_LIGHTS);
    for (int i = 0; i < lightCount; ++i)
        color += calculatePointLight(pointLights[i], normal, viewDir, baseColor);
    if (spotLight_enabled == 1)
        color += calculateSpotLight(normal, viewDir, baseColor);

    color += baseColor * emissiveStrength;

    if (foliageMaterial == 1)
        color = mix(color, baseColor * 0.72, 0.18);

    if (fogEnabled == 1)
    {
        float distanceFromCamera = length(viewPos - FragPos);
        float linearFog = smoothstep(fogStart, fogEnd, distanceFromCamera);
        float expFog = 1.0 - exp(-fogDensity * max(distanceFromCamera - fogStart, 0.0));
        float noise = fogNoise(FragPos * 0.08 + fogCenter * 0.02 + vec3(fogTime * 0.015));
        float fogAmount = clamp(max(linearFog, expFog * 0.66) + (noise - 0.5) * 0.08, 0.0, 1.0);
        color = mix(color, fogColor, fogAmount);
    }

    FragColor = vec4(color, alpha);
}
