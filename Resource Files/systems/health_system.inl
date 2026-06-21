struct JamesHealthState
{
    static constexpr int MaxHealth = 3;
    int health = MaxHealth;
    float secondsSinceDamage = 0.0f;
    float invulnerabilityTimer = 0.0f;
};

JamesHealthState jamesHealth;

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
        playerIsMoving = false;
        currentState = GAME_OVER;
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
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS ||
        glfwGetKey(window, GLFW_KEY_ENTER) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
    return true;
}
