struct StatusHud
{
    HudTexture jamesLabel;
    HudTexture angelaLabel;
    HudTexture recoverLabel;
};

StatusHud statusHud;

void initStatusHud()
{
    statusHud.jamesLabel = createTextTexture(L"JAMES", L"Georgia", 18.0f, 108, 28,
                                             Gdiplus::Color(235, 226, 214, 230));
    statusHud.angelaLabel = createTextTexture(L"ANGELA", L"Georgia", 18.0f, 126, 28,
                                              Gdiplus::Color(235, 226, 214, 230));
    statusHud.recoverLabel = createTextTexture(L"AUTO RECUP.", L"Georgia", 13.0f, 116, 22,
                                               Gdiplus::Color(190, 205, 196, 205));
}

template <typename DrawHudQuad>
void renderHealthBlocks(DrawHudQuad &drawHudQuad, float x, float y, int health, int maxHealth, const glm::vec3 &fillColor)
{
    const float blockWidth = 28.0f;
    const float blockHeight = 10.0f;
    const float gap = 6.0f;
    for (int i = 0; i < maxHealth; ++i)
    {
        const float blockX = x + static_cast<float>(i) * (blockWidth + gap);
        drawHudQuad(getWhiteTexture(), blockX, y, blockWidth, blockHeight, glm::vec3(0.10f, 0.015f, 0.012f), 0.72f);
        drawHudQuad(getWhiteTexture(), blockX, y, blockWidth, 1.0f, glm::vec3(0.70f, 0.075f, 0.055f), 0.38f);
        if (i < health)
            drawHudQuad(getWhiteTexture(), blockX + 2.0f, y + 2.0f, blockWidth - 4.0f, blockHeight - 4.0f, fillColor, 0.92f);
    }
}

template <typename DrawHudQuad>
void renderRecoveryProgress(DrawHudQuad &drawHudQuad, float x, float y)
{
    constexpr float recoverySeconds = 10.0f;
    const float width = 116.0f;
    const float height = 7.0f;
    const bool recovering = jamesHealth.health > 0 && jamesHealth.health < JamesHealthState::MaxHealth;
    const float progress = recovering ? glm::clamp(jamesHealth.secondsSinceDamage / recoverySeconds, 0.0f, 1.0f) : 1.0f;
    const glm::vec3 fillColor = recovering ? glm::vec3(0.42f, 0.68f, 0.55f) : glm::vec3(0.35f, 0.42f, 0.38f);

    drawHudQuad(statusHud.recoverLabel.texture, x - 2.0f, y - 23.0f,
                static_cast<float>(statusHud.recoverLabel.width), static_cast<float>(statusHud.recoverLabel.height), glm::vec3(1.0f), recovering ? 1.0f : 0.55f);
    drawHudQuad(getWhiteTexture(), x, y, width, height, glm::vec3(0.055f, 0.04f, 0.035f), 0.78f);
    drawHudQuad(getWhiteTexture(), x, y, width * progress, height, fillColor, recovering ? 0.88f : 0.45f);
}

template <typename DrawHudQuad>
void renderStatusHud(Shader &lightingShader, DrawHudQuad &&drawHudQuad)
{
    if (currentState != PLAYING)
        return;

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
    lightingShader.setFloat("material_ambientStrength", 1.0f);
    lightingShader.setFloat("material_specularStrength", 0.0f);
    lightingShader.setVec3("dirLight_color", 1.0f, 1.0f, 1.0f);
    lightingShader.setFloat("objectAlpha", 1.0f);
    lightingShader.setFloat("alphaCutoff", 0.01f);

    const float panelX = 18.0f;
    const float panelY = 16.0f;
    const bool showAngelaStatus = isAngelaFollowing();
    const float panelHeight = showAngelaStatus ? 154.0f : 100.0f;
    drawHudQuad(getWhiteTexture(), panelX, panelY, 164.0f, panelHeight, glm::vec3(0.020f, 0.012f, 0.012f), 0.68f);
    drawHudQuad(getWhiteTexture(), panelX, panelY, 164.0f, 2.0f, glm::vec3(0.62f, 0.06f, 0.045f), 0.55f);

    drawHudQuad(statusHud.jamesLabel.texture, panelX + 12.0f, panelY + 10.0f,
                static_cast<float>(statusHud.jamesLabel.width), static_cast<float>(statusHud.jamesLabel.height), glm::vec3(1.0f), 1.0f);
    renderHealthBlocks(drawHudQuad, panelX + 16.0f, panelY + 43.0f,
                       jamesHealth.health, JamesHealthState::MaxHealth, glm::vec3(0.82f, 0.08f, 0.055f));
    renderRecoveryProgress(drawHudQuad, panelX + 16.0f, panelY + 83.0f);

    if (showAngelaStatus)
    {
        drawHudQuad(statusHud.angelaLabel.texture, panelX + 12.0f, panelY + 100.0f,
                    static_cast<float>(statusHud.angelaLabel.width), static_cast<float>(statusHud.angelaLabel.height), glm::vec3(1.0f), isAngelaAlive() ? 1.0f : 0.45f);
        renderHealthBlocks(drawHudQuad, panelX + 16.0f, panelY + 129.0f,
                           getAngelaHealth(), getAngelaMaxHealth(), glm::vec3(0.68f, 0.18f, 0.16f));
    }

    glEnable(GL_DEPTH_TEST);
}
