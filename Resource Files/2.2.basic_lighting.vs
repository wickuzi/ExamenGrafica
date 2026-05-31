#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord;
layout (location = 3) in vec4 aBoneIds;
layout (location = 4) in vec4 aWeights;

out vec3 FragPos;
out vec3 Normal;
out vec2 TexCoord;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform int useSkinning;

const int MAX_BONES = 100;
const int MAX_BONE_INFLUENCE = 4;
uniform mat4 finalBonesMatrices[MAX_BONES];

void main()
{
    vec4 localPosition = vec4(aPos, 1.0);
    vec3 localNormal = aNormal;

    if (useSkinning == 1) {
        vec4 skinnedPosition = vec4(0.0);
        vec3 skinnedNormal = vec3(0.0);
        float totalWeight = 0.0;

        for (int i = 0; i < MAX_BONE_INFLUENCE; ++i) {
            int boneId = int(aBoneIds[i]);
            float weight = aWeights[i];
            if (boneId < 0 || weight <= 0.0)
                continue;
            if (boneId >= MAX_BONES) {
                skinnedPosition = localPosition;
                skinnedNormal = localNormal;
                totalWeight = 1.0;
                break;
            }

            mat4 boneTransform = finalBonesMatrices[boneId];
            skinnedPosition += boneTransform * localPosition * weight;
            skinnedNormal += mat3(boneTransform) * localNormal * weight;
            totalWeight += weight;
        }

        if (totalWeight > 0.0) {
            localPosition = skinnedPosition;
            localNormal = normalize(skinnedNormal);
        }
    }

    FragPos = vec3(model * localPosition);
    Normal = mat3(transpose(inverse(model))) * localNormal;
    TexCoord = aTexCoord;
    gl_Position = projection * view * model * localPosition;
}
