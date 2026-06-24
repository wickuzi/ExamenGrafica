struct JamesHealthState
{
    static constexpr int MaxHealth = 3;
    int health = MaxHealth;
    float secondsSinceDamage = 0.0f;
    float invulnerabilityTimer = 0.0f;
};

enum class GameOverCause
{
    James,
    Angela
};

struct EndingScreenState
{
    HudTexture victoryPrompt;
    HudTexture gameOverTitle;
    HudTexture gameOverSubtitle;
    HudTexture gameOverPrompt;
    unsigned int backgroundTexture = 0;
};

JamesHealthState jamesHealth;
GameOverCause gameOverCause = GameOverCause::James;
EndingScreenState endingScreen;

void initEndingScreen(const std::filesystem::path &resourceDir)
{
    std::filesystem::path endingPath = resourceDir / "ending_screen.png";
    if (!std::filesystem::exists(endingPath))
        endingPath = std::filesystem::path("Resource Files") / "ending_screen.png";

    std::map<std::string, unsigned int> loadedEndingTextures;
    if (std::filesystem::exists(endingPath))
        endingScreen.backgroundTexture = loadTextureFromFile(endingPath.string(), "", loadedEndingTextures, false);

    endingScreen.victoryPrompt = createStyledTextTexture(
        L"PRESIONA SPACE PARA VOLVER AL MENU DE INICIO",
        L"Georgia", 21.0f, 650, 44,
        Gdiplus::Color(232, 220, 190, 190), Gdiplus::Color(18, 12, 10, 210), true,
        Gdiplus::Color(0, 0, 0, 145));

    endingScreen.gameOverTitle = createStyledTextTexture(
        L"GAME OVER", L"Georgia", 62.0f, 640, 92,
        Gdiplus::Color(235, 210, 204, 232), Gdiplus::Color(70, 0, 0, 235), true,
        Gdiplus::Color(0, 0, 0, 180));
    endingScreen.gameOverSubtitle = createStyledTextTexture(
        L"La niebla te ha reclamado.",
        L"Georgia", 28.0f, 560, 58,
        Gdiplus::Color(214, 202, 194, 210), Gdiplus::Color(22, 8, 8, 220), true,
        Gdiplus::Color(0, 0, 0, 150));
    endingScreen.gameOverPrompt = createStyledTextTexture(
        L"PRESIONA ESPACIO PARA VOLVER AL MENU",
        L"Georgia", 21.0f, 560, 44,
        Gdiplus::Color(232, 210, 196, 190), Gdiplus::Color(24, 6, 6, 210), true,
        Gdiplus::Color(0, 0, 0, 145));
}

void triggerGameOver(GameOverCause cause)
{
    gameOverCause = cause;
    playerIsMoving = false;
    saveMenuOpen = false;
    stopGameplayAudio();
    currentState = GAME_OVER;
}

void triggerVictoryEnding()
{
    if (currentState != PLAYING)
        return;

    playerIsMoving = false;
    saveMenuOpen = false;
    stopGameplayAudio();
    currentState = ENDING;
}

GameOverCause getGameOverCause()
{
    return gameOverCause;
}

void resetJamesHealth()
{
    jamesHealth = JamesHealthState{};
}

bool damageJames()
{
    if (currentState != PLAYING || jamesHealth.health <= 0 || jamesHealth.invulnerabilityTimer > 0.0f)
        return false;

    --jamesHealth.health;
    playJamesHurtSound();
    jamesHealth.secondsSinceDamage = 0.0f;
    jamesHealth.invulnerabilityTimer = 0.9f;
    if (jamesHealth.health <= 0)
    {
        jamesHealth.health = 0;
        triggerGameOver(GameOverCause::James);
    }
    return true;
}

void updateJamesHealth(float deltaSeconds)
{
    if (currentState != PLAYING || jamesHealth.health <= 0)
        return;

    jamesHealth.invulnerabilityTimer = glm::max(0.0f, jamesHealth.invulnerabilityTimer - deltaSeconds);
    if (jamesHealth.health >= JamesHealthState::MaxHealth)
        return;

    jamesHealth.secondsSinceDamage += deltaSeconds;
    if (jamesHealth.secondsSinceDamage >= 10.0f)
    {
        ++jamesHealth.health;
        jamesHealth.secondsSinceDamage = 0.0f;
    }
}

bool processHealthStateInput(GLFWwindow *window)
{
    if (currentState != GAME_OVER && currentState != ENDING)
        return false;

    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
    {
        resetGameplayProgress();
        firstMouse = true;
        spaceWasPressed = true;
        eWasPressed = false;
        currentState = MENU;
    }
    return true;
}

template <typename DrawHudQuad>
void renderEndingBackground(Shader &lightingShader, DrawHudQuad &&drawHudQuad, const glm::vec3 &tint, float darkAlpha)
{
    glfwSetInputMode(glfwGetCurrentContext(), GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    glClearColor(0.018f, 0.024f, 0.022f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    lightingShader.use();
    lightingShader.setMat4("projection", glm::ortho(0.0f, static_cast<float>(SCR_WIDTH), static_cast<float>(SCR_HEIGHT), 0.0f, -1.0f, 1.0f));
    lightingShader.setMat4("view", glm::mat4(1.0f));
    lightingShader.setInt("fogEnabled", 0);
    lightingShader.setInt("useSkinning", 0);
    lightingShader.setInt("numPointLights", 0);
    lightingShader.setInt("spotLight_enabled", 0);
    lightingShader.setVec3("dirLight_direction", 0.0f, 0.0f, -1.0f);
    lightingShader.setVec3("dirLight_color", 1.0f, 1.0f, 1.0f);
    lightingShader.setFloat("material_ambientStrength", 1.0f);
    lightingShader.setFloat("material_specularStrength", 0.0f);
    lightingShader.setFloat("emissiveStrength", 0.0f);
    lightingShader.setFloat("alphaCutoff", 0.01f);

    if (endingScreen.backgroundTexture)
        drawHudQuad(endingScreen.backgroundTexture, 0.0f, 0.0f, static_cast<float>(SCR_WIDTH), static_cast<float>(SCR_HEIGHT), glm::vec3(1.0f), 1.0f);
    else
        drawHudQuad(getWhiteTexture(), 0.0f, 0.0f, static_cast<float>(SCR_WIDTH), static_cast<float>(SCR_HEIGHT), tint, 1.0f);

    drawHudQuad(getWhiteTexture(), 0.0f, 0.0f, static_cast<float>(SCR_WIDTH), static_cast<float>(SCR_HEIGHT), tint, darkAlpha);
}

template <typename DrawHudQuad>
void renderEndingScreen(Shader &lightingShader, DrawHudQuad &&drawHudQuad)
{
    renderEndingBackground(lightingShader, drawHudQuad, glm::vec3(0.0f), 0.0f);
    drawHudQuad(endingScreen.victoryPrompt.texture, (SCR_WIDTH - endingScreen.victoryPrompt.width) * 0.5f, 622.0f,
                static_cast<float>(endingScreen.victoryPrompt.width), static_cast<float>(endingScreen.victoryPrompt.height), glm::vec3(1.0f), 0.78f);
}
