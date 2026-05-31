#version 330 core
out vec4 FragColor;

in vec3 TexCoords;

uniform samplerCube skybox;

void main()
{
    vec3 color = texture(skybox, TexCoords).rgb;
    float gray = dot(color, vec3(0.299, 0.587, 0.114));
    color = mix(vec3(gray), color, 0.55);
    color = mix(color, vec3(0.72, 0.73, 0.66), 0.35);
    FragColor = vec4(color, 1.0);
}
