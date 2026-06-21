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
uniform int useTextureAlpha;
uniform int useWhiteChromaKey;
uniform int foliageMaterial;
uniform float emissiveStrength;

// Silent Hill style fog and grade
uniform vec3 fogColor;
uniform float fogDensity;
uniform float fogStart;
uniform float fogEnd;
uniform int fogEnabled;
uniform vec3 fogCenter;
uniform float fogTime;

// material
uniform float material_shininess;
uniform float material_specularStrength;
uniform float material_ambientStrength;

// directional light
uniform vec3 dirLight_direction;
uniform vec3 dirLight_color;

// point light
// multiple point lights
#define MAX_POINT_LIGHTS 32
struct PointLight {
    vec3 position;
    vec3 color;
    float constant;
    float linear;
    float quadratic;
};
uniform int numPointLights;
uniform PointLight pointLights[MAX_POINT_LIGHTS];

float fogHash(vec3 p)
{
    return fract(sin(dot(p, vec3(12.9898, 78.233, 37.719))) * 43758.5453);
}

// Smooth value noise avoids the square cells produced by the previous floor-only noise.
float fogNoise(vec3 p)
{
    vec3 cell = floor(p);
    vec3 f = fract(p);
    f = f * f * (3.0 - 2.0 * f);
    float n000 = fogHash(cell + vec3(0, 0, 0));
    float n100 = fogHash(cell + vec3(1, 0, 0));
    float n010 = fogHash(cell + vec3(0, 1, 0));
    float n110 = fogHash(cell + vec3(1, 1, 0));
    float n001 = fogHash(cell + vec3(0, 0, 1));
    float n101 = fogHash(cell + vec3(1, 0, 1));
    float n011 = fogHash(cell + vec3(0, 1, 1));
    float n111 = fogHash(cell + vec3(1, 1, 1));
    float nx00 = mix(n000, n100, f.x);
    float nx10 = mix(n010, n110, f.x);
    float nx01 = mix(n001, n101, f.x);
    float nx11 = mix(n011, n111, f.x);
    return mix(mix(nx00, nx10, f.y), mix(nx01, nx11, f.y), f.z);
}

float fogFbm(vec3 p)
{
    // Rotating and offsetting every octave prevents axis-aligned cubic cells.
    mat3 rotateDomain = mat3(
         0.00,  0.80,  0.60,
        -0.80,  0.36, -0.48,
        -0.60, -0.48,  0.64);
    float value = 0.0;
    float amplitude = 0.55;
    for (int octave = 0; octave < 3; ++octave)
    {
        value += fogNoise(p) * amplitude;
        p = rotateDomain * p * 2.03 + vec3(3.1, 5.7, 1.9);
        amplitude *= 0.48;
    }
    return value;
}

// spotlight
uniform vec3 spotLight_position;
uniform vec3 spotLight_direction;
uniform vec3 spotLight_color;
uniform float spotLight_cutOff; // cos(inner)
uniform float spotLight_outerCutOff; // cos(outer)
uniform int spotLight_enabled;

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
    vec3 toLight = pl.position - fragPos;
    float distance = length(toLight);
    // Keep the optimization, but fade before the cutoff so point lights never
    // leave a visible circular edge on streets or sidewalks.
    if (distance > 18.0)
        return vec3(0.0);
    float radiusFade = 1.0 - smoothstep(13.0, 18.0, distance);
    vec3 lightDir = toLight / max(distance, 0.0001);
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), material_shininess);
    float attenuation = 1.0 / (pl.constant + pl.linear * distance + pl.quadratic * (distance * distance));
    vec3 ambient = material_ambientStrength * pl.color * attenuation;
    vec3 diffuse = diff * pl.color * attenuation;
    vec3 specular = material_specularStrength * spec * pl.color * attenuation;
    // Local atmospheric spill makes authored LightPos nodes readable on roads
    // even when imported normals are coarse, and suggests light inside the fog.
    vec3 localGlow = pl.color * attenuation * 0.16;
    return (ambient + diffuse + specular + localGlow) * radiusFade;
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
    // A chest lamp also creates a faint short-range bounce on James' jacket
    // and face. It is deliberately independent of the forward cone and fades
    // quickly, so it never competes with the main beam.
    float fillAttenuation = 1.0 / (1.0 + 2.0 * distance + 4.0 * distance * distance);
    vec3 chestFill = spotLight_color * 0.16 * fillAttenuation;
    return ambient + diffuse + specular + chestFill;
}

void main()
{
    // Resolve alpha first so transparent atlas pixels skip all lighting/fog work.
    vec4 texSample = texture(texture1, TexCoord);
    float finalAlpha = (useTextureAlpha == 1) ? texSample.a * objectAlpha : objectAlpha;
    if (useWhiteChromaKey == 1) {
        float minChannel = min(texSample.r, min(texSample.g, texSample.b));
        float maxChannel = max(texSample.r, max(texSample.g, texSample.b));
        float nearWhite = smoothstep(0.82, 0.97, minChannel);
        float neutral = 1.0 - smoothstep(0.035, 0.16, maxChannel - minChannel);
        finalAlpha *= 1.0 - nearWhite * neutral;
    }
    if (finalAlpha < alphaCutoff)
        discard;

    vec3 norm = normalize(Normal);
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 resultLighting = CalcDirectionalLight(norm, viewDir);
    float lightingDistance = length(FragPos.xz - fogCenter.xz);
    if (fogEnabled == 0 || lightingDistance < fogEnd)
    {
        for (int i = 0; i < numPointLights; ++i)
            resultLighting += CalcPointLight(pointLights[i], norm, FragPos, viewDir);
        if (spotLight_enabled == 1 && fogEnabled == 1)
            resultLighting += CalcSpotLight(norm, FragPos, viewDir);
    }

    vec3 baseColor = texSample.rgb * objectColor;
    if (foliageMaterial == 1) {
        // The source atlas contains very pale leaves. Rebuild an olive range
        // from its luminance so veins and clusters remain detailed, not white.
        float leafDetail = clamp(dot(texSample.rgb, vec3(0.24, 0.68, 0.08)), 0.0, 1.0);
        vec3 leafShadow = vec3(0.055, 0.095, 0.040);
        vec3 leafLight = vec3(0.30, 0.43, 0.20);
        baseColor = mix(leafShadow, leafLight, smoothstep(0.12, 0.82, leafDetail));
    }
    vec3 result = resultLighting * baseColor;
    result += baseColor * emissiveStrength;

    // Dirty, cold grade inspired by Silent Hill while preserving albedo detail.
    float gray = dot(result, vec3(0.299, 0.587, 0.114));
    result = mix(vec3(gray), result, 0.74);
    result *= vec3(0.99, 1.00, 0.94);

    if (fogEnabled == 1) {
        // Cylindrical/radial visibility volume around James rather than a flat wall.
        vec2 radialDelta = FragPos.xz - fogCenter.xz;
        float radialDistance = length(radialDelta);
        // Two-axis wind and a slow vertical bob make motion readable while the
        // banks remain heavy and atmospheric.
        vec3 wind = vec3(fogTime * 0.060,
                         sin(fogTime * 0.22) * 0.16,
                         fogTime * 0.034 + sin(fogTime * 0.13) * 0.10);
        vec3 driftingFog = FragPos * vec3(0.105, 0.16, 0.105) + wind;
        // Domain warp makes banks curl rather than align to world-space boxes.
        float warp = fogFbm(driftingFog * 0.62 + 4.2);
        driftingFog += vec3(warp * 0.85, warp * 0.24, -warp * 0.65);
        float largeNoise = fogFbm(driftingFog);
        float fineNoise = fogFbm(driftingFog * 1.85 + 7.3);

        // Approximate volumetric integration along the view ray. Different
        // depths receive different density instead of one flat surface value.
        vec3 cameraToFragment = FragPos - viewPos;
        float volumeNoise = 0.0;
        for (int sampleIndex = 1; sampleIndex <= 3; ++sampleIndex)
        {
            float t = float(sampleIndex) / 4.0;
            vec3 samplePos = viewPos + cameraToFragment * t;
            volumeNoise += fogFbm(samplePos * vec3(0.10, 0.15, 0.10) + wind);
        }
        volumeNoise *= 0.333333;
        float rollingGust = sin(dot(FragPos.xz, vec2(0.115, 0.073)) - fogTime * 0.42 + warp * 2.8) * 0.5 + 0.5;
        float risingWisp = sin(FragPos.y * 1.15 + FragPos.x * 0.045 - fogTime * 0.31 + fineNoise * 3.4) * 0.5 + 0.5;
        float densityShape = largeNoise * 0.39 + fineNoise * 0.14 + volumeNoise * 0.29 +
                             rollingGust * 0.12 + risingWisp * 0.06;
        float variation = mix(0.68, 1.34, densityShape);
        float warpedDistance = radialDistance * variation;
        float linearFog = clamp((fogEnd - warpedDistance) / (fogEnd - fogStart), 0.0, 1.0);
        float lowFog = 1.0 + 0.20 * clamp(1.8 - FragPos.y, 0.0, 1.8);
        float effectiveFogDensity = fogDensity * (foliageMaterial == 1 ? 0.70 : 1.0);
        float expFog = exp(-effectiveFogDensity * lowFog * warpedDistance);
        float fogFactor = clamp(min(linearFog, expFog), 0.0, 1.0);
        // Modulate fog color too; dense areas retain visible cloudy texture
        // instead of collapsing into one perfectly flat gray value.
        float slowPulse = 0.96 + sin(fogTime * 0.19 + radialDistance * 0.08) * 0.04;
        vec3 texturedFogColor = fogColor * mix(0.70, 1.27, densityShape) * slowPulse;
        result = mix(texturedFogColor, result, fogFactor);
    }

    // Soft cinematic edge darkening makes the radial fog volume readable and
    // avoids the uniformly bright, flat frame of the previous pass.
    vec2 screenUv = gl_FragCoord.xy / vec2(1280.0, 720.0);
    float edge = smoothstep(0.34, 0.78, length((screenUv - 0.5) * vec2(1.0, 0.82)));
    result *= mix(1.0, 0.66, edge);

    // Subtle analog grain breaks up perfectly clean fog gradients without
    // hiding the map's materials, road markings, foliage, or decals.
    float grain = fract(sin(dot(gl_FragCoord.xy, vec2(12.9898, 78.233))) * 43758.5453) - 0.5;
    result += grain * 0.006;

    FragColor = vec4(result, finalAlpha);
}
