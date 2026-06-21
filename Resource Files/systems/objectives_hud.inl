enum class ObjectiveId
{
    FindAngela,
    FindShotgun,
    EliminateEnemies
};

struct ObjectivesHud
{
    HudTexture title;
    HudTexture angela;
    HudTexture shotgun;
    HudTexture enemies;
    HudTexture check;
    bool angelaComplete = false;
    bool shotgunComplete = false;
    bool enemiesComplete = false;
};

ObjectivesHud objectivesHud;

void initObjectivesHud()
{
    objectivesHud.title = createTextTexture(
        L"OBJETIVOS", L"Georgia", 17.0f, 135, 27,
        Gdiplus::Color(235, 225, 215, 220));
    objectivesHud.angela = createTextTexture(
        L"Encuentra a Angela", L"Georgia", 14.0f, 205, 25,
        Gdiplus::Color(225, 215, 208, 215));
    objectivesHud.shotgun = createTextTexture(
        L"Encuentra la escopeta", L"Georgia", 14.0f, 205, 25,
        Gdiplus::Color(225, 215, 208, 215));
    objectivesHud.enemies = createTextTexture(
        L"Elimina a todos los enemigos", L"Georgia", 14.0f, 235, 25,
        Gdiplus::Color(225, 215, 208, 215));
    objectivesHud.check = createTextTexture(
        L"\u2713", L"Segoe UI Symbol", 18.0f, 25, 25,
        Gdiplus::Color(255, 72, 210, 92));
}

void completeObjective(ObjectiveId objective)
{
    switch (objective)
    {
    case ObjectiveId::FindAngela:
        objectivesHud.angelaComplete = true;
        break;
    case ObjectiveId::FindShotgun:
        objectivesHud.shotgunComplete = true;
        break;
    case ObjectiveId::EliminateEnemies:
        objectivesHud.enemiesComplete = true;
        break;
    }
}

// Enemy gameplay can call this after removing the final living enemy.
void completeEnemiesObjective()
{
    completeObjective(ObjectiveId::EliminateEnemies);
}

template <typename DrawHudQuad>
void renderObjectivesHud(Shader &lightingShader, DrawHudQuad &&drawHudQuad)
{
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
    lightingShader.setVec3("dirLight_direction", 0.0f, 0.0f, -1.0f);
    lightingShader.setVec3("dirLight_color", 1.0f, 1.0f, 1.0f);
    lightingShader.setFloat("material_ambientStrength", 1.0f);
    lightingShader.setFloat("material_specularStrength", 0.0f);
    lightingShader.setFloat("objectAlpha", 1.0f);
    lightingShader.setFloat("alphaCutoff", 0.01f);

    const float panelWidth = 300.0f;
    const float panelHeight = 132.0f;
    const float panelX = static_cast<float>(SCR_WIDTH) - panelWidth - 18.0f;
    const float panelY = 16.0f;
    const float textX = panelX + 39.0f;

    drawHudQuad(getWhiteTexture(), panelX, panelY, panelWidth, panelHeight,
                glm::vec3(0.025f, 0.018f, 0.018f), 0.76f);
    drawHudQuad(getWhiteTexture(), panelX, panelY, panelWidth, 2.0f,
                glm::vec3(0.55f, 0.055f, 0.045f), 0.72f);
    drawHudQuad(objectivesHud.title.texture, panelX + 14.0f, panelY + 8.0f,
                static_cast<float>(objectivesHud.title.width), static_cast<float>(objectivesHud.title.height), glm::vec3(1.0f), 1.0f);

    const float row1 = panelY + 41.0f;
    const float row2 = panelY + 69.0f;
    const float row3 = panelY + 97.0f;
    drawHudQuad(objectivesHud.angela.texture, textX, row1,
                static_cast<float>(objectivesHud.angela.width), static_cast<float>(objectivesHud.angela.height), glm::vec3(1.0f), 1.0f);
    drawHudQuad(objectivesHud.shotgun.texture, textX, row2,
                static_cast<float>(objectivesHud.shotgun.width), static_cast<float>(objectivesHud.shotgun.height), glm::vec3(1.0f), 1.0f);
    drawHudQuad(objectivesHud.enemies.texture, textX, row3,
                static_cast<float>(objectivesHud.enemies.width), static_cast<float>(objectivesHud.enemies.height), glm::vec3(1.0f), 1.0f);

    const float checkX = panelX + 13.0f;
    if (objectivesHud.angelaComplete)
        drawHudQuad(objectivesHud.check.texture, checkX, row1, 25.0f, 25.0f, glm::vec3(1.0f), 1.0f);
    if (objectivesHud.shotgunComplete)
        drawHudQuad(objectivesHud.check.texture, checkX, row2, 25.0f, 25.0f, glm::vec3(1.0f), 1.0f);
    if (objectivesHud.enemiesComplete)
        drawHudQuad(objectivesHud.check.texture, checkX, row3, 25.0f, 25.0f, glm::vec3(1.0f), 1.0f);

    lightingShader.setFloat("objectAlpha", 1.0f);
    lightingShader.setFloat("alphaCutoff", 0.38f);
    lightingShader.setFloat("material_ambientStrength", 0.25f);
    lightingShader.setFloat("material_specularStrength", 0.05f);
    lightingShader.setInt("fogEnabled", 1);
    glEnable(GL_DEPTH_TEST);
}
