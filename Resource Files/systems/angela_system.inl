struct AngelaSystemState
{
    bool cinematicPlaying = false;
    int conversationStage = 0; // 0=not met, 1/2=dialogue lines, 3=following
    std::string cinematicPath;
    bool hasNode = false;
    glm::vec3 position = glm::vec3(0.0f);
    glm::vec3 initialPosition = glm::vec3(0.0f);
    float yaw = 0.0f;
    float initialYaw = 0.0f;

    glm::vec3 aabbMin = glm::vec3(FLT_MAX);
    glm::vec3 aabbMax = glm::vec3(-FLT_MAX);
    std::vector<MeshData> meshes;
    std::unordered_map<std::string, BoneInfo> boneInfo;
    int boneCount = 0;
    glm::mat4 globalInverseTransform = glm::mat4(1.0f);
    float renderScale = 1.0f;

    AnimationClip idleClip;
    AnimationClip walkClip;
    AnimationClip runClip;
    AnimationClip hitClip;
    AnimationState animState;
    bool shouldRun = false;
    bool shouldWalk = false;
    int health = 2;
    bool alive = true;
    bool hitReacting = false;
    bool recoveringFromHit = false;
    float hitRecoveryTimer = 0.0f;
    std::vector<glm::mat4> hitRecoveryPose;

    HudTexture interactText;
    HudTexture dialogueAngelaText;
    HudTexture dialogueJamesText;
};

AngelaSystemState angelaSystem;

const float ANGELA_INTERACT_RADIUS = 1.65f;
const float ANGELA_GROUND_OFFSET = 1.83f;
const float ANGELA_FOLLOW_CLOSER_FACTOR = 0.70f;
const float ANGELA_HIT_RECOVERY_SECONDS = 0.24f;
const int ANGELA_MAX_HEALTH = 2;

bool hasAngelaSystemNode()
{
    return angelaSystem.hasNode;
}

glm::vec3 getAngelaPosition()
{
    return angelaSystem.position;
}

bool isAngelaDialogueVisible()
{
    return angelaSystem.conversationStage == 1 || angelaSystem.conversationStage == 2;
}

bool isAngelaMeetLightActive()
{
    return angelaSystem.hasNode && angelaSystem.conversationStage == 0;
}

bool isAngelaFollowing()
{
    return angelaSystem.hasNode && angelaSystem.alive && angelaSystem.conversationStage >= 3;
}

bool isAngelaAlive()
{
    return angelaSystem.alive;
}

void resetAngelaHealth()
{
    angelaSystem.health = ANGELA_MAX_HEALTH;
    angelaSystem.alive = true;
    angelaSystem.hitReacting = false;
    angelaSystem.recoveringFromHit = false;
    angelaSystem.hitRecoveryTimer = 0.0f;
}

void resetAngelaProgress()
{
    resetAngelaHealth();
    angelaSystem.cinematicPlaying = false;
    angelaSystem.conversationStage = 0;
    angelaSystem.position = angelaSystem.initialPosition;
    angelaSystem.yaw = angelaSystem.initialYaw;
    angelaSystem.shouldRun = false;
    angelaSystem.shouldWalk = false;
    angelaSystem.animState.current = angelaSystem.idleClip.valid ? &angelaSystem.idleClip : nullptr;
    angelaSystem.animState.currentTime = 0.0f;
}

int getAngelaHealth()
{
    return angelaSystem.health;
}

int getAngelaMaxHealth()
{
    return ANGELA_MAX_HEALTH;
}

bool isAngelaCinematicPlaying()
{
    return angelaSystem.cinematicPlaying;
}

bool isAngelaInRange()
{
    return angelaSystem.hasNode && angelaSystem.conversationStage == 0 &&
        glm::length(glm::vec2(playerPosition.x - angelaSystem.position.x,
                              playerPosition.z - angelaSystem.position.z)) <= ANGELA_INTERACT_RADIUS;
}

void initAngelaHudTextures()
{
    angelaSystem.interactText = createTextTexture(L"PRESS E TO INTERACT", L"Georgia", 18.0f, 245, 34,
                                                  Gdiplus::Color(245, 230, 220, 218));
    angelaSystem.dialogueAngelaText = createTextTexture(
        L"MARY: YO TAMBIEN ESTOY PERDIDA, HE VISTO A CRIATURAS EXTRANAS POR ESTOS LADOS, NO SE SI SEA SEGURO.   [ESPACIO]",
        L"Georgia", 20.0f, 1010, 72, Gdiplus::Color(245, 230, 220, 230));
    angelaSystem.dialogueJamesText = createTextTexture(
        L"JAMES: ESTA BIEN, SIGUEME Y SALDREMOS DE ACA.   [ESPACIO]",
        L"Georgia", 20.0f, 760, 54, Gdiplus::Color(245, 230, 220, 230));
}

void initAngelaSystem(const std::filesystem::path &resourceDir)
{
    angelaSystem.health = ANGELA_MAX_HEALTH;
    angelaSystem.alive = true;
    std::filesystem::path angelaDir = resourceDir.parent_path() / "models" / "angela";
    if (!std::filesystem::exists(angelaDir))
        angelaDir = resourceDir.parent_path().parent_path() / "models" / "angela";

    const std::filesystem::path angelaModelPath = angelaDir / "Angela.fbx";
    const std::filesystem::path angelaIdlePath = angelaDir / "animations" / "angela_idle.fbx";
    const std::filesystem::path angelaWalkPath = angelaDir / "animations" / "angela_walk.fbx";
    const std::filesystem::path angelaRunPath = angelaDir / "animations" / "Slow Run.fbx";
    const std::filesystem::path angelaHitPath = angelaDir / "animations" / "Getting Hit.fbx";
    angelaSystem.cinematicPath = std::filesystem::absolute(angelaDir / "video" / "jamesmeetsangela.wmv").string();
    angelaSystem.animState.finalMatrices.assign(MAX_BONES, glm::mat4(1.0f));

    if (std::filesystem::exists(angelaModelPath))
    {
        allowSkinnedTextureSearch = true;
        const std::filesystem::path &angelaSkinnedModelPath = std::filesystem::exists(angelaIdlePath)
            ? angelaIdlePath : angelaModelPath;
        angelaSystem.meshes = loadModel(angelaSkinnedModelPath.string(), angelaSystem.aabbMin, angelaSystem.aabbMax,
                                        &angelaSystem.boneInfo, &angelaSystem.boneCount,
                                        &angelaSystem.globalInverseTransform);
        allowSkinnedTextureSearch = false;

        const float angelaHeight = glm::max(angelaSystem.aabbMax.y - angelaSystem.aabbMin.y, 0.001f);
        angelaSystem.renderScale = 1.68f / angelaHeight;
        std::cout << "Loaded Angela meshes: " << angelaSystem.meshes.size()
                  << " bones=" << angelaSystem.boneCount
                  << " scale=" << angelaSystem.renderScale << std::endl;
    }

    if (std::filesystem::exists(angelaIdlePath))
    {
        angelaSystem.idleClip = loadAnimationClip(angelaIdlePath.string(), "angela_idle");
        if (angelaSystem.idleClip.valid)
            angelaSystem.animState.current = &angelaSystem.idleClip;
    }
    if (std::filesystem::exists(angelaRunPath))
        angelaSystem.runClip = loadAnimationClip(angelaRunPath.string(), "angela_slow_run");
    if (std::filesystem::exists(angelaWalkPath))
        angelaSystem.walkClip = loadAnimationClip(angelaWalkPath.string(), "angela_walk");
    if (std::filesystem::exists(angelaHitPath))
        angelaSystem.hitClip = loadAnimationClip(angelaHitPath.string(), "angela_getting_hit");
}

void setAngelaInitialTransform(bool hasNode, const glm::mat4 &mapModelTransform, const glm::mat4 &angelaNodeTransform)
{
    angelaSystem.hasNode = hasNode;
    if (!hasNode)
        return;

    const glm::mat4 angelaWorld = mapModelTransform * angelaNodeTransform;
    angelaSystem.position = glm::vec3(angelaWorld[3]);
    glm::vec3 angelaForward = glm::vec3(angelaWorld * glm::vec4(0.0f, 0.0f, 1.0f, 0.0f));
    if (glm::length(glm::vec2(angelaForward.x, angelaForward.z)) > 0.001f)
        angelaSystem.yaw = glm::degrees(atan2(angelaForward.x, angelaForward.z));

    float angelaGroundY = angelaSystem.position.y;
    if (findAnyGroundHeightAt(angelaSystem.position.x, angelaSystem.position.z, angelaGroundY))
        angelaSystem.position.y = angelaGroundY + ANGELA_GROUND_OFFSET;

    std::cout << "Angela initial position: (" << angelaSystem.position.x << ", "
              << angelaSystem.position.y << ", " << angelaSystem.position.z
              << ") yaw=" << angelaSystem.yaw << std::endl;
    angelaSystem.initialPosition = angelaSystem.position;
    angelaSystem.initialYaw = angelaSystem.yaw;
}

void finishAngelaCinematic()
{
    closeCinematicPlayer();
    angelaSystem.cinematicPlaying = false;
    currentState = PLAYING;
    angelaSystem.conversationStage = 1;
    spaceWasPressed = true;
    firstMouse = true;
}

bool damageAngela()
{
    if (!angelaSystem.alive || currentState != PLAYING)
        return false;

    playAngelaCrySound();
    if (angelaSystem.hitClip.valid)
    {
        angelaSystem.hitReacting = true;
        angelaSystem.recoveringFromHit = false;
        angelaSystem.hitRecoveryTimer = 0.0f;
        angelaSystem.animState.current = nullptr;
    }

    --angelaSystem.health;
    if (angelaSystem.health <= 0)
    {
        angelaSystem.health = 0;
        angelaSystem.alive = false;
        triggerGameOver(GameOverCause::Angela);
        std::cout << "Angela died. Game over." << std::endl;
        return true;
    }
    std::cout << "Angela hit. HP=" << angelaSystem.health << std::endl;
    return false;
}

const AnimationClip *desiredAngelaMovementClip()
{
    if (angelaSystem.shouldRun && angelaSystem.runClip.valid)
        return &angelaSystem.runClip;
    if (angelaSystem.shouldWalk && angelaSystem.walkClip.valid)
        return &angelaSystem.walkClip;
    return &angelaSystem.idleClip;
}

void applyAngelaSkinScale()
{
    for (auto &matrix : angelaSystem.animState.finalMatrices)
    {
        matrix = glm::translate(glm::mat4(1.0f), JAMES_FBX_SKIN_OFFSET) *
                 glm::scale(glm::mat4(1.0f), glm::vec3(JAMES_FBX_SKIN_SCALE)) *
                 matrix;
    }
}

void blendAngelaRecoveryPose(float alpha)
{
    if (angelaSystem.hitRecoveryPose.size() != angelaSystem.animState.finalMatrices.size())
        return;

    for (size_t i = 0; i < angelaSystem.animState.finalMatrices.size(); ++i)
    {
        glm::mat4 blended(1.0f);
        for (int column = 0; column < 4; ++column)
            blended[column] = glm::mix(angelaSystem.hitRecoveryPose[i][column],
                                       angelaSystem.animState.finalMatrices[i][column], alpha);
        angelaSystem.animState.finalMatrices[i] = blended;
    }
}

void updateAngelaFollower(GLFWwindow *window, float deltaSeconds)
{
    angelaSystem.shouldRun = false;
    angelaSystem.shouldWalk = false;
    if (!isAngelaFollowing())
        return;

    const float yawRadians = glm::radians(playerYaw);
    const glm::vec3 jamesForward(sinf(yawRadians), 0.0f, cosf(yawRadians));
    const glm::vec3 jamesRight(jamesForward.z, 0.0f, -jamesForward.x);
    const glm::vec3 followTarget = playerPosition -
        jamesForward * (1.15f * ANGELA_FOLLOW_CLOSER_FACTOR) +
        jamesRight * (0.38f * ANGELA_FOLLOW_CLOSER_FACTOR);
    glm::vec3 toTarget = followTarget - angelaSystem.position;
    toTarget.y = 0.0f;
    const float followDistance = glm::length(toTarget);
    const bool jamesRunning = playerIsMoving && glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS;
    angelaSystem.shouldRun = jamesRunning;
    angelaSystem.shouldWalk = playerIsMoving && !jamesRunning;

    if (playerIsMoving && followDistance > 0.55f * ANGELA_FOLLOW_CLOSER_FACTOR)
    {
        const glm::vec3 direction = toTarget / followDistance;
        const float followSpeed = jamesRunning ? 3.55f : 1.65f;
        glm::vec3 nextAngelaPosition = angelaSystem.position + direction * glm::min(followDistance, followSpeed * deltaSeconds);
        float followerGroundY = nextAngelaPosition.y - ANGELA_GROUND_OFFSET;
        if (findWalkAreaHeightAt(nextAngelaPosition.x, nextAngelaPosition.z, followerGroundY) ||
            findAnyGroundHeightAt(nextAngelaPosition.x, nextAngelaPosition.z, followerGroundY))
        {
            nextAngelaPosition.y = followerGroundY + ANGELA_GROUND_OFFSET;
            angelaSystem.position = nextAngelaPosition;
        }
        angelaSystem.yaw = glm::degrees(atan2(direction.x, direction.z));
    }
}

void updateAngelaAnimation(float deltaSeconds)
{
    if (!angelaSystem.hasNode)
        return;

    if (angelaSystem.hitReacting && angelaSystem.hitClip.valid)
    {
        updateAnimation(angelaSystem.animState, &angelaSystem.hitClip, deltaSeconds, angelaSystem.boneInfo,
                        angelaSystem.boneCount, angelaSystem.globalInverseTransform, false);
        applyAngelaSkinScale();
        if (angelaSystem.animState.currentTime >= glm::max(0.0f, angelaSystem.hitClip.duration - 1.01f))
        {
            angelaSystem.hitReacting = false;
            angelaSystem.recoveringFromHit = true;
            angelaSystem.hitRecoveryTimer = 0.0f;
            angelaSystem.hitRecoveryPose = angelaSystem.animState.finalMatrices;
            angelaSystem.animState.current = nullptr;
        }
        return;
    }

    const AnimationClip *desiredAngelaClip = desiredAngelaMovementClip();
    if (desiredAngelaClip->valid)
    {
        updateAnimation(angelaSystem.animState, desiredAngelaClip, deltaSeconds, angelaSystem.boneInfo,
                        angelaSystem.boneCount, angelaSystem.globalInverseTransform, true);
        applyAngelaSkinScale();

        if (angelaSystem.recoveringFromHit)
        {
            angelaSystem.hitRecoveryTimer += deltaSeconds;
            const float alpha = glm::clamp(angelaSystem.hitRecoveryTimer / ANGELA_HIT_RECOVERY_SECONDS, 0.0f, 1.0f);
            blendAngelaRecoveryPose(alpha);
            if (alpha >= 1.0f)
                angelaSystem.recoveringFromHit = false;
        }
    }
}

void renderAngelaSystem(Shader &lightingShader)
{
    if (!angelaSystem.hasNode || angelaSystem.meshes.empty())
        return;

    const glm::vec3 angelaCenter = (angelaSystem.aabbMin + angelaSystem.aabbMax) * 0.5f;
    glm::mat4 angelaModel(1.0f);
    angelaModel = glm::translate(angelaModel, angelaSystem.position);
    angelaModel = glm::rotate(angelaModel, glm::radians(angelaSystem.yaw), glm::vec3(0.0f, 1.0f, 0.0f));
    angelaModel = glm::scale(angelaModel, glm::vec3(angelaSystem.renderScale));
    angelaModel = glm::translate(angelaModel, glm::vec3(-angelaCenter.x, -angelaSystem.aabbMin.y, -angelaCenter.z));
    lightingShader.setMat4("model", angelaModel);
    lightingShader.setInt("useSkinning", angelaSystem.boneCount > 0 && angelaSystem.animState.current ? 1 : 0);

    for (int i = 0; i < glm::min(angelaSystem.boneCount, MAX_BONES); ++i)
        lightingShader.setMat4("finalBonesMatrices[" + std::to_string(i) + "]", angelaSystem.animState.finalMatrices[i]);

    for (MeshData &mesh : angelaSystem.meshes)
    {
        lightingShader.setVec3("objectColor", mesh.materialColor);
        lightingShader.setFloat("objectAlpha", mesh.materialAlpha);
        lightingShader.setInt("useTextureAlpha", 1);
        lightingShader.setInt("useWhiteChromaKey", 0);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, mesh.texture);
        glBindVertexArray(mesh.VAO);
        glDrawElements(GL_TRIANGLES, (GLsizei)mesh.indexCount, GL_UNSIGNED_INT, 0);
    }
    lightingShader.setVec3("objectColor", 1.0f, 1.0f, 1.0f);
    lightingShader.setFloat("objectAlpha", 1.0f);
    lightingShader.setInt("useSkinning", 0);
}

template <typename DrawHudQuad>
bool renderAngelaHud(DrawHudQuad &drawHudQuad)
{
    if (isAngelaDialogueVisible())
    {
        const HudTexture &line = angelaSystem.conversationStage == 1
            ? angelaSystem.dialogueAngelaText : angelaSystem.dialogueJamesText;
        const float panelWidth = glm::min(static_cast<float>(line.width + 36), 1120.0f);
        drawHudQuad(getWhiteTexture(), 70.0f, 590.0f, panelWidth, 92.0f, glm::vec3(0.08f, 0.0f, 0.0f), 0.78f);
        drawHudQuad(getWhiteTexture(), 70.0f, 590.0f, panelWidth, 2.0f, glm::vec3(0.70f, 0.08f, 0.06f), 0.55f);
        drawHudQuad(line.texture, 88.0f, 602.0f, static_cast<float>(line.width), static_cast<float>(line.height), glm::vec3(1.0f), 1.0f);
        return true;
    }

    if (isAngelaInRange())
    {
        drawHudQuad(getWhiteTexture(), 530.0f, 628.0f, 285.0f, 32.0f, glm::vec3(0.12f, 0.0f, 0.0f), 0.48f);
        drawHudQuad(getWhiteTexture(), 530.0f, 628.0f, 285.0f, 1.0f, glm::vec3(0.62f, 0.08f, 0.06f), 0.42f);
        drawHudQuad(angelaSystem.interactText.texture, 548.0f, 630.0f,
                    static_cast<float>(angelaSystem.interactText.width),
                    static_cast<float>(angelaSystem.interactText.height), glm::vec3(1.0f), 1.0f);
        return true;
    }

    return false;
}

bool processAngelaDialogueInput(bool dialogueSpacePressed, bool ePressed)
{
    if (!isAngelaDialogueVisible())
        return false;

    if (dialogueSpacePressed && !spaceWasPressed)
    {
        ++angelaSystem.conversationStage;
        if (angelaSystem.conversationStage >= 3)
            checkObjectiveVictory();
    }
    spaceWasPressed = dialogueSpacePressed;
    eWasPressed = ePressed;
    return true;
}

bool processAngelaInteractionInput(GLFWwindow *window, bool ePressed)
{
    const float angelaDistance = (angelaSystem.hasNode && angelaSystem.conversationStage == 0)
        ? glm::length(glm::vec2(playerPosition.x - angelaSystem.position.x,
                                playerPosition.z - angelaSystem.position.z))
        : FLT_MAX;
    if (!ePressed || eWasPressed || angelaDistance > ANGELA_INTERACT_RADIUS)
        return false;

    playInteractionSound();
    completeObjective(ObjectiveId::FindAngela);
    glfwHWND = GetActiveWindow();
    if (playCinematicVideo(angelaSystem.cinematicPath, glfwHWND))
    {
        angelaSystem.cinematicPlaying = true;
        currentState = CINEMATIC;
    }
    else
    {
        angelaSystem.conversationStage = 1;
        spaceWasPressed = true;
    }
    eWasPressed = true;
    return true;
}

void shutdownAngelaSystem()
{
    for (MeshData &mesh : angelaSystem.meshes)
    {
        if (mesh.VAO) glDeleteVertexArrays(1, &mesh.VAO);
        if (mesh.VBO) glDeleteBuffers(1, &mesh.VBO);
        if (mesh.EBO) glDeleteBuffers(1, &mesh.EBO);
        if (mesh.texture) glDeleteTextures(1, &mesh.texture);
    }
    angelaSystem.meshes.clear();
}
