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

JamesHealthState jamesHealth;
GameOverCause gameOverCause = GameOverCause::James;

void triggerGameOver(GameOverCause cause)
{
    gameOverCause = cause;
    playerIsMoving = false;
    saveMenuOpen = false;
    stopGameplayMusic();
    currentState = GAME_OVER;
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
    if (currentState != GAME_OVER)
        return false;
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
    {
        resetJamesHealth();
        resetAngelaHealth();
        firstMouse = true;
        spaceWasPressed = true;
        eWasPressed = false;
        currentState = MENU;
    }
    return true;
}
