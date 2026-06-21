struct ChestFlashlight
{
    std::vector<MeshData> meshes;
    glm::vec3 aabbMin = glm::vec3(FLT_MAX);
    glm::vec3 aabbMax = glm::vec3(-FLT_MAX);
    float renderScale = 1.0f;
    bool loaded = false;
};

ChestFlashlight chestFlashlight;

void initChestFlashlight(const std::filesystem::path &resourceDir)
{
    const std::vector<std::filesystem::path> candidates = {
        std::filesystem::path("models") / "james" / "linterna.glb",
        std::filesystem::path("..") / "models" / "james" / "linterna.glb",
        resourceDir.parent_path() / "models" / "james" / "linterna.glb",
        resourceDir.parent_path().parent_path() / "models" / "james" / "linterna.glb"};

    for (const auto &path : candidates)
    {
        if (!std::filesystem::exists(path))
            continue;
        chestFlashlight.meshes = loadModel(path.string(), chestFlashlight.aabbMin, chestFlashlight.aabbMax);
        if (!chestFlashlight.meshes.empty())
        {
            const glm::vec3 size = chestFlashlight.aabbMax - chestFlashlight.aabbMin;
            const float largestSide = glm::max(glm::max(size.x, size.y), glm::max(size.z, 0.001f));
            chestFlashlight.renderScale = 0.18f / largestSide;
            chestFlashlight.loaded = true;
            std::cout << "Loaded James chest flashlight: " << path.string() << std::endl;
        }
        break;
    }
}

glm::vec3 jamesFlashlightForward()
{
    const float yaw = glm::radians(playerYaw);
    return glm::normalize(glm::vec3(sinf(yaw), -0.035f, cosf(yaw)));
}

void updateChestFlashlightLight(Shader &lightingShader)
{
    const glm::vec3 forward = jamesFlashlightForward();
    const glm::vec3 right(forward.z, 0.0f, -forward.x);
    const glm::vec3 lightPosition = playerPosition + glm::vec3(0.0f, 1.38f, 0.0f) +
                                    forward * 0.20f - right * 0.11f;
    lightingShader.setInt("spotLight_enabled", chestFlashlight.loaded ? 1 : 0);
    lightingShader.setVec3("spotLight_position", lightPosition);
    lightingShader.setVec3("spotLight_direction", forward);
    lightingShader.setVec3("spotLight_color", 2.15f, 2.02f, 1.55f);
    lightingShader.setFloat("spotLight_cutOff", cosf(glm::radians(13.0f)));
    lightingShader.setFloat("spotLight_outerCutOff", cosf(glm::radians(22.0f)));
}

void renderChestFlashlight(Shader &lightingShader, const glm::mat4 &jamesWorld,
                           const std::unordered_map<std::string, BoneInfo> &boneInfo,
                           const AnimationState &animationState)
{
    if (!chestFlashlight.loaded)
        return;

    glm::vec3 chestPosition = playerPosition + glm::vec3(0.0f, 1.38f, 0.0f);
    const std::array<const char *, 3> chestBones = {
        "mixamorig:Spine2", "mixamorig:Spine1", "mixamorig:Spine"};
    for (const char *boneName : chestBones)
    {
        const auto bone = boneInfo.find(boneName);
        if (bone == boneInfo.end() || bone->second.id < 0 ||
            bone->second.id >= static_cast<int>(animationState.finalMatrices.size()))
            continue;
        const glm::mat4 animatedBone = animationState.finalMatrices[bone->second.id] *
                                       glm::inverse(bone->second.offset);
        chestPosition = glm::vec3(jamesWorld * animatedBone * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));
        break;
    }

    const glm::vec3 forward = jamesFlashlightForward();
    const glm::vec3 right(forward.z, 0.0f, -forward.x);
    chestPosition += forward * 0.16f - right * 0.11f;

    const glm::vec3 center = (chestFlashlight.aabbMin + chestFlashlight.aabbMax) * 0.5f;
    glm::mat4 model(1.0f);
    model = glm::translate(model, chestPosition);
    model = glm::rotate(model, glm::radians(playerYaw), glm::vec3(0.0f, 1.0f, 0.0f));
    // linterna.glb has its long/lens axis baked diagonally by its source-node
    // transforms. Align that real axis with James' local +Z (his front).
    const glm::vec3 authoredLensAxis = glm::normalize(glm::vec3(0.683f, -0.120f, -0.720f));
    const glm::quat faceForward = glm::rotation(authoredLensAxis, glm::vec3(0.0f, 0.0f, -1.0f));
    model *= glm::toMat4(faceForward);
    model = glm::scale(model, glm::vec3(chestFlashlight.renderScale));
    model = glm::translate(model, -center);

    lightingShader.setMat4("model", model);
    lightingShader.setInt("useSkinning", 0);
    lightingShader.setFloat("alphaCutoff", 0.01f);
    lightingShader.setFloat("emissiveStrength", 0.18f);
    for (auto &mesh : chestFlashlight.meshes)
    {
        lightingShader.setVec3("objectColor", mesh.materialColor);
        lightingShader.setFloat("objectAlpha", mesh.materialAlpha);
        lightingShader.setInt("useTextureAlpha", mesh.useTextureAlpha ? 1 : 0);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, mesh.texture);
        glBindVertexArray(mesh.VAO);
        glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(mesh.indexCount), GL_UNSIGNED_INT, 0);
    }
    lightingShader.setVec3("objectColor", 1.0f, 1.0f, 1.0f);
    lightingShader.setFloat("objectAlpha", 1.0f);
    lightingShader.setInt("useTextureAlpha", 1);
    lightingShader.setFloat("alphaCutoff", 0.38f);
    lightingShader.setFloat("emissiveStrength", 0.0f);
}

void shutdownChestFlashlight()
{
    for (auto &mesh : chestFlashlight.meshes)
    {
        if (mesh.VAO) glDeleteVertexArrays(1, &mesh.VAO);
        if (mesh.VBO) glDeleteBuffers(1, &mesh.VBO);
        if (mesh.EBO) glDeleteBuffers(1, &mesh.EBO);
        if (mesh.texture) glDeleteTextures(1, &mesh.texture);
    }
    chestFlashlight.meshes.clear();
}
