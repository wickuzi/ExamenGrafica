struct DamageOverlay
{
    unsigned int vignetteTexture = 0;
    HudTexture deathTitle;
    HudTexture deathPrompt;
};

DamageOverlay damageOverlay;

void initDamageOverlay()
{
    const int width = 512;
    const int height = 288;
    std::vector<unsigned char> pixels(width * height * 4, 0);
    for (int y = 0; y < height; ++y)
    {
        for (int x = 0; x < width; ++x)
        {
            const float nx = (static_cast<float>(x) / (width - 1) - 0.5f) * 2.0f;
            const float ny = (static_cast<float>(y) / (height - 1) - 0.5f) * 2.0f;
            const float edge = glm::smoothstep(0.38f, 1.05f, glm::max(glm::abs(nx), glm::abs(ny)));
            const float corners = glm::smoothstep(0.50f, 1.22f, glm::length(glm::vec2(nx, ny)));
            const float irregular = 0.88f + 0.12f * sinf(x * 0.071f + y * 0.043f);
            const unsigned char alpha = static_cast<unsigned char>(255.0f * glm::clamp(glm::max(edge, corners) * irregular, 0.0f, 1.0f));
            const size_t index = (static_cast<size_t>(y) * width + x) * 4;
            pixels[index + 0] = 125;
            pixels[index + 1] = 0;
            pixels[index + 2] = 0;
            pixels[index + 3] = alpha;
        }
    }

    glGenTextures(1, &damageOverlay.vignetteTexture);
    glBindTexture(GL_TEXTURE_2D, damageOverlay.vignetteTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());

    damageOverlay.deathTitle = createTextTexture(
        L"JAMES HA MUERTO", L"Georgia", 52.0f, 620, 82,
        Gdiplus::Color(245, 205, 25, 22));
    damageOverlay.deathPrompt = createTextTexture(
        L"PRESIONA ENTER O ESCAPE PARA SALIR", L"Georgia", 20.0f, 520, 42,
        Gdiplus::Color(225, 205, 190, 185));
}

template <typename DrawHudQuad>
void prepareDamageOverlayShader(Shader &lightingShader)
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
    lightingShader.setFloat("material_ambientStrength", 1.0f);
    lightingShader.setVec3("dirLight_color", 1.0f, 1.0f, 1.0f);
    lightingShader.setFloat("objectAlpha", 1.0f);
    lightingShader.setFloat("alphaCutoff", 0.01f);
}

template <typename DrawHudQuad>
void renderLowHealthOverlay(Shader &lightingShader, DrawHudQuad &&drawHudQuad)
{
    if (jamesHealth.health != 1 || currentState != PLAYING)
        return;
    prepareDamageOverlayShader<DrawHudQuad>(lightingShader);
    const float pulse = 0.76f + 0.12f * sinf(static_cast<float>(glfwGetTime()) * 3.2f);
    drawHudQuad(damageOverlay.vignetteTexture, 0.0f, 0.0f,
                static_cast<float>(SCR_WIDTH), static_cast<float>(SCR_HEIGHT), glm::vec3(1.0f), pulse);
    glEnable(GL_DEPTH_TEST);
}

template <typename DrawHudQuad>
void renderGameOverScreen(Shader &lightingShader, DrawHudQuad &&drawHudQuad)
{
    glClearColor(0.012f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    prepareDamageOverlayShader<DrawHudQuad>(lightingShader);
    drawHudQuad(damageOverlay.vignetteTexture, 0.0f, 0.0f,
                static_cast<float>(SCR_WIDTH), static_cast<float>(SCR_HEIGHT), glm::vec3(1.0f), 1.0f);
    drawHudQuad(damageOverlay.deathTitle.texture,
                (SCR_WIDTH - damageOverlay.deathTitle.width) * 0.5f, 280.0f,
                static_cast<float>(damageOverlay.deathTitle.width), static_cast<float>(damageOverlay.deathTitle.height), glm::vec3(1.0f), 1.0f);
    drawHudQuad(damageOverlay.deathPrompt.texture,
                (SCR_WIDTH - damageOverlay.deathPrompt.width) * 0.5f, 390.0f,
                static_cast<float>(damageOverlay.deathPrompt.width), static_cast<float>(damageOverlay.deathPrompt.height), glm::vec3(1.0f), 1.0f);
}

void shutdownDamageOverlay()
{
    if (damageOverlay.vignetteTexture)
        glDeleteTextures(1, &damageOverlay.vignetteTexture);
}
