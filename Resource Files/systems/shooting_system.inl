struct ShootingState
{
    bool mouseWasPressed = false;
    bool shotFiredThisFrame = false;
    bool animationActive = false;
    float animationTimer = 0.0f;
    float animationDuration = 2.0f;
    float muzzleFlashTimer = 0.0f;
    HudTexture reloadingText;
};

ShootingState shootingState;

void configureShootingSystem(const AnimationClip *fireClip)
{
    (void)fireClip;
    shootingState.animationDuration = 2.0f;
    shootingState.reloadingText = createTextTexture(
        L"RECARGANDO...", L"Georgia", 19.0f, 220, 36,
        Gdiplus::Color(235, 205, 185, 175));
}

void updateShootingSystem(GLFWwindow *window, float deltaSeconds)
{
    shootingState.shotFiredThisFrame = false;
    shootingState.muzzleFlashTimer = glm::max(0.0f, shootingState.muzzleFlashTimer - deltaSeconds);
    if (shootingState.animationActive)
    {
        shootingState.animationTimer -= deltaSeconds;
        if (shootingState.animationTimer <= 0.0f)
        {
            shootingState.animationTimer = 0.0f;
            shootingState.animationActive = false;
        }
    }

    const bool mousePressed = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
    const bool newClick = mousePressed && !shootingState.mouseWasPressed;
    if (newClick && shotgunCollected && !playerIsMoving && !shootingState.animationActive && currentState == PLAYING)
    {
        shootingState.shotFiredThisFrame = true;
        shootingState.animationActive = true;
        shootingState.animationTimer = shootingState.animationDuration;
        shootingState.muzzleFlashTimer = 0.11f;
        playShotgunSound();
    }
    shootingState.mouseWasPressed = mousePressed;
}

bool didJamesFireThisFrame()
{
    return shootingState.shotFiredThisFrame;
}

bool isShotAnimationActive()
{
    return shootingState.animationActive;
}

bool isShotgunMuzzleFlashActive()
{
    return shootingState.muzzleFlashTimer > 0.0f;
}

void uploadShotgunMuzzleFlash(Shader &lightingShader, int &pointIndex, int maxPointLights)
{
    if (!isShotgunMuzzleFlashActive() || pointIndex >= maxPointLights)
        return;

    const float yaw = glm::radians(playerYaw);
    const glm::vec3 forward(sinf(yaw), 0.0f, cosf(yaw));
    const glm::vec3 right(forward.z, 0.0f, -forward.x);
    const glm::vec3 muzzlePosition = playerPosition + glm::vec3(0.0f, 1.24f, 0.0f) +
                                     forward * 0.92f + right * 0.10f;
    const float strength = glm::clamp(shootingState.muzzleFlashTimer / 0.11f, 0.0f, 1.0f);
    const std::string base = "pointLights[" + std::to_string(pointIndex) + "].";
    lightingShader.setVec3(base + "position", muzzlePosition);
    lightingShader.setVec3(base + "color", glm::vec3(5.2f, 2.65f, 0.38f) * strength);
    lightingShader.setFloat(base + "constant", 1.0f);
    lightingShader.setFloat(base + "linear", 0.32f);
    lightingShader.setFloat(base + "quadratic", 0.20f);
    ++pointIndex;
}

template <typename DrawHudQuad>
void renderShootingCrosshair(Shader &lightingShader, DrawHudQuad &&drawHudQuad)
{
    if (!shotgunCollected || playerIsMoving || currentState != PLAYING)
        return;

    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    lightingShader.use();
    lightingShader.setMat4("projection", glm::ortho(0.0f, static_cast<float>(SCR_WIDTH), static_cast<float>(SCR_HEIGHT), 0.0f, -1.0f, 1.0f));
    lightingShader.setMat4("view", glm::mat4(1.0f));
    lightingShader.setInt("fogEnabled", 0);
    lightingShader.setInt("useSkinning", 0);
    lightingShader.setInt("numPointLights", 0);
    lightingShader.setFloat("material_ambientStrength", 1.0f);
    lightingShader.setVec3("dirLight_color", 1.0f, 1.0f, 1.0f);
    lightingShader.setFloat("alphaCutoff", 0.01f);

    const float centerX = static_cast<float>(SCR_WIDTH) * 0.5f;
    const float centerY = static_cast<float>(SCR_HEIGHT) * 0.5f;
    if (shootingState.animationActive)
    {
        const float pulse = 0.72f + 0.20f * sinf(static_cast<float>(glfwGetTime()) * 7.0f);
        drawHudQuad(shootingState.reloadingText.texture,
                    centerX - shootingState.reloadingText.width * 0.5f, centerY + 46.0f,
                    static_cast<float>(shootingState.reloadingText.width),
                    static_cast<float>(shootingState.reloadingText.height), glm::vec3(1.0f), pulse);
        lightingShader.setFloat("alphaCutoff", 0.38f);
        glEnable(GL_DEPTH_TEST);
        return;
    }
    const glm::vec3 color(0.82f, 0.16f, 0.13f);
    drawHudQuad(getWhiteTexture(), centerX - 14.0f, centerY - 1.0f, 9.0f, 2.0f, color, 0.88f);
    drawHudQuad(getWhiteTexture(), centerX + 5.0f, centerY - 1.0f, 9.0f, 2.0f, color, 0.88f);
    drawHudQuad(getWhiteTexture(), centerX - 1.0f, centerY - 14.0f, 2.0f, 9.0f, color, 0.88f);
    drawHudQuad(getWhiteTexture(), centerX - 1.0f, centerY + 5.0f, 2.0f, 9.0f, color, 0.88f);

    lightingShader.setFloat("alphaCutoff", 0.38f);
    glEnable(GL_DEPTH_TEST);
}
