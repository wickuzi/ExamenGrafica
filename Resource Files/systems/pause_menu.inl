struct PauseMenu
{
    HudTexture title;
    HudTexture resume;
    HudTexture resumeSelected;
    HudTexture exit;
    HudTexture exitSelected;
    unsigned int frozenFrameTexture = 0;
    int selectedOption = 0;
    bool waitingForEscapeRelease = false;
    bool mouseWasPressed = false;
    bool enterWasPressed = false;
    bool upWasPressed = false;
    bool downWasPressed = false;
};

PauseMenu pauseMenu;

void initPauseMenu()
{
    pauseMenu.title = createStyledTextTexture(
        L"PAUSA", L"Georgia", 52.0f, 430, 78,
        Gdiplus::Color(245, 225, 220, 225), Gdiplus::Color(65, 0, 0, 235), true,
        Gdiplus::Color(150, 0, 0, 150));
    pauseMenu.resume = createTextTexture(
        L"REANUDAR", L"Georgia", 29.0f, 390, 58,
        Gdiplus::Color(225, 210, 205, 215));
    pauseMenu.resumeSelected = createStyledTextTexture(
        L"REANUDAR", L"Georgia", 32.0f, 410, 62,
        Gdiplus::Color(255, 225, 220, 245), Gdiplus::Color(95, 0, 0, 240), true,
        Gdiplus::Color(235, 20, 12, 125));
    pauseMenu.exit = createTextTexture(
        L"SALIR AL MENU PRINCIPAL", L"Georgia", 25.0f, 440, 58,
        Gdiplus::Color(215, 200, 195, 205));
    pauseMenu.exitSelected = createStyledTextTexture(
        L"SALIR AL MENU PRINCIPAL", L"Georgia", 27.0f, 460, 62,
        Gdiplus::Color(255, 220, 215, 240), Gdiplus::Color(95, 0, 0, 240), true,
        Gdiplus::Color(235, 20, 12, 125));
    glGenTextures(1, &pauseMenu.frozenFrameTexture);
    glBindTexture(GL_TEXTURE_2D, pauseMenu.frozenFrameTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
}

void capturePauseBackground()
{
    glBindTexture(GL_TEXTURE_2D, pauseMenu.frozenFrameTexture);
    glReadBuffer(GL_FRONT);
    glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, SCR_WIDTH, SCR_HEIGHT);
    glReadBuffer(GL_BACK);
}

void openPauseMenu(GLFWwindow *window)
{
    if (currentState != PLAYING)
        return;
    capturePauseBackground();
    currentState = PAUSED;
    playerIsMoving = false;
    pauseMenu.selectedOption = 0;
    pauseMenu.waitingForEscapeRelease = true;
    pauseMenu.mouseWasPressed = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
    pauseMenu.enterWasPressed = glfwGetKey(window, GLFW_KEY_ENTER) == GLFW_PRESS;
    pauseMenu.upWasPressed = false;
    pauseMenu.downWasPressed = false;
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
}

void resumeFromPause(GLFWwindow *window)
{
    currentState = PLAYING;
    firstMouse = true;
    pauseMenu.waitingForEscapeRelease = true;
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
}

void exitPauseToMainMenu(GLFWwindow *window)
{
    stopGameplayMusic();
    currentState = MENU;
    selectedItemIndex = 0;
    firstMouse = true;
    mouseLeftWasPressed = true;
    pauseMenu.waitingForEscapeRelease = true;
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
}

bool processPauseMenuInput(GLFWwindow *window)
{
    if (currentState != PAUSED)
        return false;

    const bool escapePressed = glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS;
    if (pauseMenu.waitingForEscapeRelease)
    {
        if (!escapePressed)
            pauseMenu.waitingForEscapeRelease = false;
    }
    else if (escapePressed)
    {
        resumeFromPause(window);
        return true;
    }

    const float buttonX = (static_cast<float>(SCR_WIDTH) - 460.0f) * 0.5f;
    const float resumeY = 312.0f;
    const float exitY = 402.0f;
    if (mouseX >= buttonX && mouseX <= buttonX + 460.0f)
    {
        if (mouseY >= resumeY && mouseY <= resumeY + 66.0f)
            pauseMenu.selectedOption = 0;
        else if (mouseY >= exitY && mouseY <= exitY + 66.0f)
            pauseMenu.selectedOption = 1;
    }

    const bool upPressed = glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS ||
                           glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS;
    const bool downPressed = glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS ||
                             glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS;
    if (upPressed && !pauseMenu.upWasPressed)
        pauseMenu.selectedOption = 0;
    if (downPressed && !pauseMenu.downWasPressed)
        pauseMenu.selectedOption = 1;
    pauseMenu.upWasPressed = upPressed;
    pauseMenu.downWasPressed = downPressed;

    const bool mousePressed = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
    const bool clicked = mousePressed && !pauseMenu.mouseWasPressed &&
        mouseX >= buttonX && mouseX <= buttonX + 460.0f &&
        ((pauseMenu.selectedOption == 0 && mouseY >= resumeY && mouseY <= resumeY + 66.0f) ||
         (pauseMenu.selectedOption == 1 && mouseY >= exitY && mouseY <= exitY + 66.0f));
    pauseMenu.mouseWasPressed = mousePressed;

    const bool enterPressed = glfwGetKey(window, GLFW_KEY_ENTER) == GLFW_PRESS;
    const bool confirmed = clicked || (enterPressed && !pauseMenu.enterWasPressed);
    pauseMenu.enterWasPressed = enterPressed;
    if (confirmed)
    {
        playInteractionSound();
        if (pauseMenu.selectedOption == 0)
            resumeFromPause(window);
        else
            exitPauseToMainMenu(window);
    }
    return true;
}

template <typename DrawHudQuad>
void renderPauseMenu(GLFWwindow *window, Shader &lightingShader, DrawHudQuad &&drawHudQuad)
{
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    glClearColor(0.018f, 0.0f, 0.0f, 1.0f);
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

    // Negative height corrects the framebuffer texture's bottom-left origin.
    drawHudQuad(pauseMenu.frozenFrameTexture, 0.0f, static_cast<float>(SCR_HEIGHT),
                static_cast<float>(SCR_WIDTH), -static_cast<float>(SCR_HEIGHT), glm::vec3(0.48f, 0.20f, 0.20f), 0.48f);
    drawHudQuad(getWhiteTexture(), 0.0f, 0.0f, static_cast<float>(SCR_WIDTH), static_cast<float>(SCR_HEIGHT),
                glm::vec3(0.16f, 0.0f, 0.0f), 0.76f);
    drawHudQuad(getWhiteTexture(), 255.0f, 118.0f, 770.0f, 485.0f,
                glm::vec3(0.035f, 0.0f, 0.0f), 0.90f);
    drawHudQuad(getWhiteTexture(), 255.0f, 118.0f, 770.0f, 3.0f,
                glm::vec3(0.95f, 0.025f, 0.018f), 0.82f);
    drawHudQuad(getWhiteTexture(), 280.0f, 142.0f, 720.0f, 1.0f,
                glm::vec3(0.42f, 0.02f, 0.018f), 0.70f);

    drawHudQuad(pauseMenu.title.texture, (SCR_WIDTH - pauseMenu.title.width) * 0.5f, 168.0f,
                static_cast<float>(pauseMenu.title.width), static_cast<float>(pauseMenu.title.height), glm::vec3(1.0f), 1.0f);

    const float pulse = 0.78f + 0.16f * sinf(static_cast<float>(glfwGetTime()) * 5.0f);
    const float optionX = (static_cast<float>(SCR_WIDTH) - 460.0f) * 0.5f;
    if (pauseMenu.selectedOption == 0)
        drawHudQuad(getWhiteTexture(), optionX, 312.0f, 460.0f, 66.0f, glm::vec3(0.56f, 0.015f, 0.01f), 0.38f * pulse);
    else
        drawHudQuad(getWhiteTexture(), optionX, 402.0f, 460.0f, 66.0f, glm::vec3(0.56f, 0.015f, 0.01f), 0.38f * pulse);

    HudTexture &resumeTexture = pauseMenu.selectedOption == 0 ? pauseMenu.resumeSelected : pauseMenu.resume;
    HudTexture &exitTexture = pauseMenu.selectedOption == 1 ? pauseMenu.exitSelected : pauseMenu.exit;
    drawHudQuad(resumeTexture.texture, (SCR_WIDTH - resumeTexture.width) * 0.5f, 314.0f,
                static_cast<float>(resumeTexture.width), static_cast<float>(resumeTexture.height), glm::vec3(1.0f), 1.0f);
    drawHudQuad(exitTexture.texture, (SCR_WIDTH - exitTexture.width) * 0.5f, 404.0f,
                static_cast<float>(exitTexture.width), static_cast<float>(exitTexture.height), glm::vec3(1.0f), 1.0f);
}

void shutdownPauseMenu()
{
    if (pauseMenu.frozenFrameTexture)
        glDeleteTextures(1, &pauseMenu.frozenFrameTexture);
    const unsigned int textures[] = {
        pauseMenu.title.texture, pauseMenu.resume.texture, pauseMenu.resumeSelected.texture,
        pauseMenu.exit.texture, pauseMenu.exitSelected.texture};
    glDeleteTextures(static_cast<GLsizei>(sizeof(textures) / sizeof(textures[0])), textures);
}
