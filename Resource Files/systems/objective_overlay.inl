struct ObjectiveOverlay
{
    HudTexture title;
    HudTexture body;
    HudTexture prompt;
    bool needsSpaceRelease = true;
};

ObjectiveOverlay objectiveOverlay;

void initObjectiveOverlay()
{
    objectiveOverlay.title = createTextTexture(
        L"OBJETIVO DEL PASEO VIRTUAL", L"Georgia", 38.0f, 680, 62,
        Gdiplus::Color(245, 232, 220, 220));
    objectiveOverlay.body = createTextTexture(
        L"ESTAS EN SILENT HILL. TU OBJETIVO ES ENCONTRAR UNA SALIDA.\n\n"
        L"EXPLORA EL MAPA Y BUSCA LA ESCOPETA PARA DEFENDERTE.\n"
        L"ENCUENTRA Y RESCATA A ANGELA.\n\n"
        L"TEN CUIDADO: HAY CRIATURAS ACECHANDO ENTRE LA NIEBLA.",
        L"Georgia", 25.0f, 900, 230, Gdiplus::Color(235, 218, 214, 205));
    objectiveOverlay.prompt = createTextTexture(
        L"PRESIONA ESPACIO PARA COMENZAR", L"Georgia", 22.0f, 520, 46,
        Gdiplus::Color(245, 190, 75, 70));
}

void showObjectiveBriefing()
{
    currentState = OBJECTIVE_BRIEFING;
    objectiveOverlay.needsSpaceRelease = true;
    firstMouse = true;
}

bool processObjectiveOverlayInput(GLFWwindow *window)
{
    if (currentState != OBJECTIVE_BRIEFING)
        return false;

    const bool spacePressed = glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS;
    if (objectiveOverlay.needsSpaceRelease)
    {
        if (!spacePressed)
            objectiveOverlay.needsSpaceRelease = false;
    }
    else if (spacePressed)
    {
        enterPlayingState();
        spaceWasPressed = true;
    }

    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
    return true;
}

template <typename DrawHudQuad>
void renderObjectiveOverlay(GLFWwindow *window, Shader &lightingShader, DrawHudQuad &&drawHudQuad)
{
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    glClearColor(0.012f, 0.014f, 0.013f, 1.0f);
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
    lightingShader.setVec3("dirLight_direction", 0.0f, 0.0f, -1.0f);
    lightingShader.setVec3("dirLight_color", 1.0f, 1.0f, 1.0f);
    lightingShader.setFloat("material_ambientStrength", 1.0f);
    lightingShader.setFloat("material_specularStrength", 0.0f);

    const float panelX = 145.0f;
    const float panelY = 120.0f;
    const float panelWidth = 950.0f;
    const float panelHeight = 480.0f;
    drawHudQuad(getWhiteTexture(), panelX, panelY, panelWidth, panelHeight, glm::vec3(0.035f, 0.025f, 0.025f), 0.96f);
    drawHudQuad(getWhiteTexture(), panelX, panelY, panelWidth, 3.0f, glm::vec3(0.62f, 0.06f, 0.045f), 0.82f);
    drawHudQuad(objectiveOverlay.title.texture, (SCR_WIDTH - objectiveOverlay.title.width) * 0.5f, 162.0f,
                static_cast<float>(objectiveOverlay.title.width), static_cast<float>(objectiveOverlay.title.height), glm::vec3(1.0f), 1.0f);
    drawHudQuad(objectiveOverlay.body.texture, (SCR_WIDTH - objectiveOverlay.body.width) * 0.5f, 250.0f,
                static_cast<float>(objectiveOverlay.body.width), static_cast<float>(objectiveOverlay.body.height), glm::vec3(1.0f), 1.0f);
    drawHudQuad(objectiveOverlay.prompt.texture, (SCR_WIDTH - objectiveOverlay.prompt.width) * 0.5f, 530.0f,
                static_cast<float>(objectiveOverlay.prompt.width), static_cast<float>(objectiveOverlay.prompt.height), glm::vec3(1.0f), 1.0f);
}
