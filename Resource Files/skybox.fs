#version 330 core
out vec4 FragColor;

in vec3 TexCoords;

uniform samplerCube skybox;
uniform float fogTime;

float hash3(vec3 p)
{
    return fract(sin(dot(p, vec3(17.17, 43.71, 91.13))) * 43758.5453);
}

float fogValueNoise(vec3 p)
{
    vec3 i = floor(p);
    vec3 f = fract(p);
    f = f * f * (3.0 - 2.0 * f);
    float a = mix(hash3(i), hash3(i + vec3(1,0,0)), f.x);
    float b = mix(hash3(i + vec3(0,1,0)), hash3(i + vec3(1,1,0)), f.x);
    float c = mix(hash3(i + vec3(0,0,1)), hash3(i + vec3(1,0,1)), f.x);
    float d = mix(hash3(i + vec3(0,1,1)), hash3(i + vec3(1,1,1)), f.x);
    return mix(mix(a, b, f.y), mix(c, d, f.y), f.z);
}

float skyFogFbm(vec3 p)
{
    mat3 rotateDomain = mat3(
         0.00,  0.80,  0.60,
        -0.80,  0.36, -0.48,
        -0.60, -0.48,  0.64);
    float value = 0.0;
    float amplitude = 0.55;
    for (int octave = 0; octave < 3; ++octave)
    {
        value += fogValueNoise(p) * amplitude;
        p = rotateDomain * p * 2.03 + vec3(2.7, 5.1, 1.3);
        amplitude *= 0.48;
    }
    return value;
}

void main()
{
    vec3 direction = normalize(TexCoords);
    vec3 drift = direction * 4.2 + vec3(fogTime * 0.052,
                                        sin(fogTime * 0.17) * 0.12,
                                        fogTime * 0.029);
    float domainWarp = skyFogFbm(drift * 0.55 + 3.4);
    drift += vec3(domainWarp * 0.75, domainWarp * 0.22, -domainWarp * 0.58);
    float broad = skyFogFbm(drift);
    float detail = skyFogFbm(drift * 1.95 + 5.7);
    float skyGust = sin(direction.x * 7.0 + direction.z * 4.0 - fogTime * 0.34 + broad * 3.0) * 0.5 + 0.5;
    float clouds = broad * 0.62 + detail * 0.23 + skyGust * 0.15;

    vec3 source = texture(skybox, TexCoords).rgb;
    float gray = dot(source, vec3(0.299, 0.587, 0.114));
    vec3 sourceGraded = mix(vec3(gray), source, 0.22);
    vec3 fogDark = vec3(0.09, 0.12, 0.105);
    vec3 fogLight = vec3(0.23, 0.26, 0.235);
    vec3 fogLayer = mix(fogDark, fogLight, smoothstep(0.22, 0.82, clouds));
    vec3 color = mix(sourceGraded, fogLayer, 0.82);

    vec2 screenUv = gl_FragCoord.xy / vec2(1280.0, 720.0);
    float edge = smoothstep(0.30, 0.76, length((screenUv - 0.5) * vec2(1.0, 0.82)));
    color *= mix(0.84, 0.34, edge);
    FragColor = vec4(color, 1.0);
}
