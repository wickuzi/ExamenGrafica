enum class PyramidHeadMode
{
    Idle,
    Patrolling,
    Chasing,
    Attacking
};

enum class PyramidHeadTarget
{
    James,
    Angela
};

struct PyramidHeadEnemy
{
    glm::vec3 position = glm::vec3(0.0f);
    float yaw = 0.0f;
    int health = 5;
    bool alive = true;
    PyramidHeadMode mode = PyramidHeadMode::Idle;
    float attackTimer = 0.0f;
    float attackDuration = 1.6f;
    bool dealtAttackDamage = false;
    PyramidHeadTarget target = PyramidHeadTarget::James;
    bool forcedJamesFocus = false;
    float targetDecisionTimer = 0.0f;
    glm::vec3 patrolTarget = glm::vec3(0.0f);
    bool hasPatrolTarget = false;
    float stuckTimer = 0.0f;
    glm::vec3 previousPosition = glm::vec3(0.0f);
    AnimationState animation;
};

struct PyramidHeadSystem
{
    std::vector<MeshData> meshes;
    glm::vec3 aabbMin = glm::vec3(FLT_MAX);
    glm::vec3 aabbMax = glm::vec3(-FLT_MAX);
    std::unordered_map<std::string, BoneInfo> bones;
    int boneCount = 0;
    glm::mat4 globalInverse = glm::mat4(1.0f);
    std::unordered_map<std::string, AnimationClip> clips;
    std::vector<PyramidHeadEnemy> enemies;
    float renderScale = 1.0f;
    bool loaded = false;
    bool fireWasPressed = false;
    std::mt19937 randomGenerator;
};

PyramidHeadSystem pyramidHeads;
const int PYRAMID_HEAD_ENEMY_COUNT = 1;
const float PYRAMID_HEAD_DETECTION_RADIUS = 14.0f;
const float PYRAMID_HEAD_LOSE_RADIUS = 18.0f;
const float PYRAMID_HEAD_ATTACK_RADIUS = 1.65f;
const float PYRAMID_HEAD_CHASE_SPEED = 2.5f;
const float PYRAMID_HEAD_PATROL_SPEED = 2.2f;

int getAlivePyramidHeadCount()
{
    int count = 0;
    for (const PyramidHeadEnemy &enemy : pyramidHeads.enemies)
    {
        if (enemy.alive)
            ++count;
    }
    return count;
}

int getTotalPyramidHeadCount()
{
    return PYRAMID_HEAD_ENEMY_COUNT;
}

glm::vec3 randomPatrolTarget(const glm::vec3 &origin)
{
    if (walkAreas.empty())
        return origin;
    std::uniform_int_distribution<size_t> areaDistribution(0, walkAreas.size() - 1);
    std::uniform_real_distribution<float> barycentric(0.10f, 0.90f);
    for (int attempt = 0; attempt < 60; ++attempt)
    {
        const WalkArea &area = walkAreas[areaDistribution(pyramidHeads.randomGenerator)];
        float u = barycentric(pyramidHeads.randomGenerator);
        float v = barycentric(pyramidHeads.randomGenerator);
        if (u + v > 1.0f)
        {
            u = 1.0f - u;
            v = 1.0f - v;
        }
        glm::vec3 target = area.a + (area.b - area.a) * u + (area.c - area.a) * v;
        target.y = area.floorY;
        if (glm::length(glm::vec2(target.x - origin.x, target.z - origin.z)) >= 9.0f)
            return target;
    }
    return origin;
}

glm::vec3 randomEnemySpawn(std::mt19937 &generator, const std::vector<glm::vec3> &usedPositions)
{
    if (walkAreas.empty())
        return playerPosition + glm::vec3(8.0f, 0.0f, 8.0f);

    std::uniform_int_distribution<size_t> areaDistribution(0, walkAreas.size() - 1);
    std::uniform_real_distribution<float> barycentric(0.12f, 0.88f);
    for (int attempt = 0; attempt < 100; ++attempt)
    {
        const WalkArea &area = walkAreas[areaDistribution(generator)];
        float u = barycentric(generator);
        float v = barycentric(generator);
        if (u + v > 1.0f)
        {
            u = 1.0f - u;
            v = 1.0f - v;
        }
        glm::vec3 candidate = area.a + (area.b - area.a) * u + (area.c - area.a) * v;
        candidate.y = area.floorY;
        if (glm::length(glm::vec2(candidate.x - playerPosition.x, candidate.z - playerPosition.z)) < 13.0f)
            continue;
        bool separated = true;
        for (const glm::vec3 &used : usedPositions)
        {
            if (glm::length(glm::vec2(candidate.x - used.x, candidate.z - used.z)) < 12.0f)
                separated = false;
        }
        if (separated)
            return candidate;
    }

    const WalkArea &fallback = walkAreas[areaDistribution(generator)];
    return glm::vec3((fallback.a.x + fallback.b.x + fallback.c.x) / 3.0f, fallback.floorY,
                     (fallback.a.z + fallback.b.z + fallback.c.z) / 3.0f);
}

void spawnPyramidHeadEnemies()
{
    pyramidHeads.enemies.clear();
    std::vector<glm::vec3> usedPositions;
    for (int i = 0; i < PYRAMID_HEAD_ENEMY_COUNT; ++i)
    {
        PyramidHeadEnemy enemy;
        enemy.position = randomEnemySpawn(pyramidHeads.randomGenerator, usedPositions);
        usedPositions.push_back(enemy.position);
        enemy.previousPosition = enemy.position;
        enemy.patrolTarget = randomPatrolTarget(enemy.position);
        enemy.hasPatrolTarget = true;
        enemy.mode = PyramidHeadMode::Patrolling;
        enemy.animation.finalMatrices.assign(MAX_BONES, glm::mat4(1.0f));
        enemy.animation.current = findClip(pyramidHeads.clips, "idle");
        const AnimationClip *attack = findClip(pyramidHeads.clips, "attack");
        enemy.attackDuration = attack
            ? glm::clamp(attack->duration / glm::max(attack->ticksPerSecond, 1.0f), 1.15f, 2.6f)
            : 1.6f;
        pyramidHeads.enemies.push_back(std::move(enemy));
    }
}

void resetPyramidHeadProgress()
{
    if (!pyramidHeads.loaded)
        return;

    spawnPyramidHeadEnemies();
    pyramidHeads.fireWasPressed = false;
    setPyramidInterferenceActive(false);
}

void initPyramidHeadSystem(const std::filesystem::path &resourceDir)
{
    std::filesystem::path enemyDir = resourceDir.parent_path() / "models" / "enemies" / "pyramidhead";
    if (!std::filesystem::exists(enemyDir))
        enemyDir = resourceDir.parent_path().parent_path() / "models" / "enemies" / "pyramidhead";
    if (!std::filesystem::exists(enemyDir))
        enemyDir = std::filesystem::path("models") / "enemies" / "pyramidhead";

    // The standalone Pyramid Head FBX is a static mesh with no skin weights.
    // The idle export contains the same character plus its complete Mixamo rig,
    // making it the correct render/base skeleton for all three clips.
    const std::filesystem::path modelPath = enemyDir / "animations" / "Standing Idle.fbx";
    if (!std::filesystem::exists(modelPath))
    {
        std::cout << "Pyramid Head model not found: " << modelPath.string() << std::endl;
        return;
    }

    const bool previousTextureSearch = allowSkinnedTextureSearch;
    allowSkinnedTextureSearch = true;
    pyramidHeads.meshes = loadModel(modelPath.string(), pyramidHeads.aabbMin, pyramidHeads.aabbMax,
                                    &pyramidHeads.bones, &pyramidHeads.boneCount, &pyramidHeads.globalInverse);
    allowSkinnedTextureSearch = previousTextureSearch;
    if (pyramidHeads.meshes.empty())
        return;

    const std::vector<std::pair<std::string, std::string>> animationFiles = {
        {"idle", "Standing Idle.fbx"},
        {"walk", "Unarmed Walk Forward.fbx"},
        {"attack", "Standing Melee Attack Horizontal.fbx"}};
    for (const auto &entry : animationFiles)
    {
        const std::filesystem::path animationPath = enemyDir / "animations" / entry.second;
        AnimationClip clip = loadAnimationClip(animationPath.string(), entry.first);
        if (clip.valid)
        {
            int matchingChannels = 0;
            for (const auto &channel : clip.channels)
                matchingChannels += pyramidHeads.bones.count(channel.first) ? 1 : 0;
            std::cout << "Pyramid Head animation '" << entry.first << "': channels="
                      << clip.channels.size() << " matchingBones=" << matchingChannels << std::endl;
            pyramidHeads.clips[entry.first] = std::move(clip);
        }
    }

    const float modelHeight = glm::max(pyramidHeads.aabbMax.y - pyramidHeads.aabbMin.y, 0.001f);
    // Slightly taller than James (1.78 m), without the exaggerated boss scale.
    pyramidHeads.renderScale = 2.32f / modelHeight;
    pyramidHeads.loaded = findClip(pyramidHeads.clips, "idle") &&
                          findClip(pyramidHeads.clips, "walk") &&
                          findClip(pyramidHeads.clips, "attack");
    if (!pyramidHeads.loaded)
    {
        std::cout << "Pyramid Head is missing one or more required animations." << std::endl;
        return;
    }

    pyramidHeads.randomGenerator.seed(static_cast<unsigned int>(GetTickCount64()));
    spawnPyramidHeadEnemies();
    std::cout << "Spawned " << PYRAMID_HEAD_ENEMY_COUNT << " Pyramid Head enemies." << std::endl;
}

void damagePyramidHeadFromShotgun()
{
    if (didJamesFireThisFrame())
    {
        PyramidHeadEnemy *bestTarget = nullptr;
        float bestDistance = FLT_MAX;
        const glm::vec3 rayDirection = glm::normalize(camera.Front);
        for (PyramidHeadEnemy &enemy : pyramidHeads.enemies)
        {
            if (!enemy.alive)
                continue;
            const glm::vec3 target = enemy.position + glm::vec3(0.0f, 1.15f, 0.0f);
            const glm::vec3 toTarget = target - camera.Position;
            const float alongRay = glm::dot(toTarget, rayDirection);
            if (alongRay <= 0.0f || alongRay > 22.0f)
                continue;
            const float missDistance = glm::length(toTarget - rayDirection * alongRay);
            if (missDistance <= 0.95f && alongRay < bestDistance)
            {
                bestDistance = alongRay;
                bestTarget = &enemy;
            }
        }

        if (bestTarget)
        {
            playPyramidHeadHitSound();
            bestTarget->target = PyramidHeadTarget::James;
            bestTarget->forcedJamesFocus = true;
            bestTarget->mode = PyramidHeadMode::Chasing;
            bestTarget->animation.current = nullptr;
            --bestTarget->health;
            if (bestTarget->health <= 0)
            {
                bestTarget->alive = false;
                bool anyAlive = false;
                for (const PyramidHeadEnemy &enemy : pyramidHeads.enemies)
                    anyAlive = anyAlive || enemy.alive;
                if (!anyAlive)
                    completeEnemiesObjective();
            }
        }
    }
}

glm::vec3 pyramidTargetPosition(const PyramidHeadEnemy &enemy)
{
    return enemy.target == PyramidHeadTarget::Angela ? getAngelaPosition() : playerPosition;
}

void updatePyramidTarget(PyramidHeadEnemy &enemy, float jamesDistance, float angelaDistance, float deltaSeconds)
{
    if (enemy.forcedJamesFocus)
    {
        enemy.target = PyramidHeadTarget::James;
        if (jamesDistance > PYRAMID_HEAD_LOSE_RADIUS)
            enemy.forcedJamesFocus = false;
        return;
    }

    enemy.targetDecisionTimer = glm::max(0.0f, enemy.targetDecisionTimer - deltaSeconds);
    if (!isAngelaFollowing())
    {
        enemy.target = PyramidHeadTarget::James;
        return;
    }

    const bool angelaIsCloser = angelaDistance < jamesDistance;
    const bool angelaIsReachable = angelaDistance <= PYRAMID_HEAD_DETECTION_RADIUS ||
        (enemy.mode == PyramidHeadMode::Chasing && angelaDistance <= PYRAMID_HEAD_LOSE_RADIUS);
    if (!angelaIsCloser || !angelaIsReachable)
    {
        enemy.target = PyramidHeadTarget::James;
        enemy.targetDecisionTimer = 0.0f;
        return;
    }

    if (enemy.targetDecisionTimer <= 0.0f)
    {
        std::uniform_int_distribution<int> focusChance(0, 99);
        enemy.target = focusChance(pyramidHeads.randomGenerator) < 55
            ? PyramidHeadTarget::Angela : PyramidHeadTarget::James;
        enemy.targetDecisionTimer = 1.25f;
    }
}

void damagePyramidTarget(PyramidHeadEnemy &enemy, float targetDistance)
{
    if (targetDistance > PYRAMID_HEAD_ATTACK_RADIUS + 0.45f)
        return;

    if (enemy.target == PyramidHeadTarget::Angela && isAngelaFollowing())
        damageAngela();
    else
        damageJames();
}

void updatePyramidHeadSystem(float deltaSeconds)
{
    if (!pyramidHeads.loaded || currentState != PLAYING)
        return;

    damagePyramidHeadFromShotgun();
    bool anyEnemyPursuing = false;

    for (PyramidHeadEnemy &enemy : pyramidHeads.enemies)
    {
        if (!enemy.alive)
            continue;
        glm::vec3 toJames = playerPosition - enemy.position;
        toJames.y = 0.0f;
        const float jamesDistance = glm::length(toJames);
        glm::vec3 toAngela = getAngelaPosition() - enemy.position;
        toAngela.y = 0.0f;
        const float angelaDistance = isAngelaFollowing() ? glm::length(toAngela) : FLT_MAX;
        updatePyramidTarget(enemy, jamesDistance, angelaDistance, deltaSeconds);

        glm::vec3 toTarget = pyramidTargetPosition(enemy) - enemy.position;
        toTarget.y = 0.0f;
        const float distance = glm::length(toTarget);
        const glm::vec3 direction = distance > 0.001f ? toTarget / distance : glm::vec3(0.0f, 0.0f, 1.0f);

        if (enemy.mode == PyramidHeadMode::Attacking)
        {
            enemy.attackTimer += deltaSeconds;
            if (!enemy.dealtAttackDamage && enemy.attackTimer >= enemy.attackDuration * 0.52f)
            {
                damagePyramidTarget(enemy, distance);
                enemy.dealtAttackDamage = true;
            }
            if (enemy.attackTimer >= enemy.attackDuration)
            {
                enemy.mode = distance <= PYRAMID_HEAD_ATTACK_RADIUS ? PyramidHeadMode::Attacking : PyramidHeadMode::Chasing;
                enemy.attackTimer = 0.0f;
                enemy.dealtAttackDamage = false;
                enemy.animation.current = nullptr;
            }
        }
        else if (distance <= PYRAMID_HEAD_ATTACK_RADIUS)
        {
            enemy.mode = PyramidHeadMode::Attacking;
            enemy.attackTimer = 0.0f;
            enemy.dealtAttackDamage = false;
            enemy.animation.current = nullptr;
        }
        else if (distance <= PYRAMID_HEAD_DETECTION_RADIUS ||
                 (enemy.mode == PyramidHeadMode::Chasing && distance <= PYRAMID_HEAD_LOSE_RADIUS))
        {
            enemy.mode = PyramidHeadMode::Chasing;
            enemy.yaw = glm::degrees(atan2(direction.x, direction.z));
            glm::vec3 nextPosition = enemy.position + direction * PYRAMID_HEAD_CHASE_SPEED * deltaSeconds;
            float groundY = enemy.position.y;
            if (findWalkAreaHeightAt(nextPosition.x, nextPosition.z, groundY) &&
                !isBlockedByCollisionBoxes(nextPosition))
            {
                nextPosition.y = groundY;
                enemy.position = nextPosition;
            }
        }
        else
        {
            enemy.mode = PyramidHeadMode::Patrolling;
            if (!enemy.hasPatrolTarget ||
                glm::length(glm::vec2(enemy.patrolTarget.x - enemy.position.x,
                                      enemy.patrolTarget.z - enemy.position.z)) < 1.0f ||
                enemy.stuckTimer > 1.5f)
            {
                enemy.patrolTarget = randomPatrolTarget(enemy.position);
                enemy.hasPatrolTarget = true;
                enemy.stuckTimer = 0.0f;
            }

            glm::vec3 toPatrolTarget = enemy.patrolTarget - enemy.position;
            toPatrolTarget.y = 0.0f;
            const float patrolDistance = glm::length(toPatrolTarget);
            if (patrolDistance > 0.001f)
            {
                const glm::vec3 patrolDirection = toPatrolTarget / patrolDistance;
                enemy.yaw = glm::degrees(atan2(patrolDirection.x, patrolDirection.z));
                glm::vec3 nextPosition = enemy.position + patrolDirection * PYRAMID_HEAD_PATROL_SPEED * deltaSeconds;
                float groundY = enemy.position.y;
                if (findWalkAreaHeightAt(nextPosition.x, nextPosition.z, groundY) &&
                    !isBlockedByCollisionBoxes(nextPosition))
                {
                    nextPosition.y = groundY;
                    enemy.position = nextPosition;
                }
            }
        }

        const float frameMovement = glm::length(glm::vec2(enemy.position.x - enemy.previousPosition.x,
                                                           enemy.position.z - enemy.previousPosition.z));
        if (enemy.mode == PyramidHeadMode::Patrolling && frameMovement < 0.001f)
            enemy.stuckTimer += deltaSeconds;
        else
            enemy.stuckTimer = 0.0f;
        enemy.previousPosition = enemy.position;

        const AnimationClip *clip = findClip(pyramidHeads.clips,
            enemy.mode == PyramidHeadMode::Idle ? "idle" :
            (enemy.mode == PyramidHeadMode::Patrolling || enemy.mode == PyramidHeadMode::Chasing) ? "walk" : "attack");
        updateAnimation(enemy.animation, clip, deltaSeconds, pyramidHeads.bones,
                        pyramidHeads.boneCount, pyramidHeads.globalInverse,
                        enemy.mode != PyramidHeadMode::Attacking);
        anyEnemyPursuing = anyEnemyPursuing || enemy.mode == PyramidHeadMode::Chasing ||
                           enemy.mode == PyramidHeadMode::Attacking;
        // Mixamo FBX animation transforms are authored in centimeters while
        // the rendered world uses meters.
        for (glm::mat4 &matrix : enemy.animation.finalMatrices)
            matrix = glm::scale(glm::mat4(1.0f), glm::vec3(0.01f)) * matrix;
    }
    setPyramidInterferenceActive(anyEnemyPursuing);
}

int activePyramidHeadAuraCount()
{
    int count = 0;
    for (const PyramidHeadEnemy &enemy : pyramidHeads.enemies)
    {
        if (enemy.alive && glm::length(enemy.position - camera.Position) <= renderDistance)
            ++count;
    }
    return count;
}

void uploadPyramidHeadAuras(Shader &lightingShader, int &pointIndex, int maxPointLights, float timeSeconds)
{
    int auraNumber = 0;
    for (const PyramidHeadEnemy &enemy : pyramidHeads.enemies)
    {
        if (!enemy.alive || glm::length(enemy.position - camera.Position) > renderDistance || pointIndex >= maxPointLights)
            continue;
        HorrorLight aura{
            enemy.position + glm::vec3(0.0f, 1.05f, 0.0f),
            glm::vec3(0.78f, 0.018f, 0.012f),
            0.92f, 4.6f, true, 6.1f + static_cast<float>(auraNumber) * 2.37f};
        sendHorrorLightToShader(lightingShader, pointIndex, aura, timeSeconds);
        ++pointIndex;
        ++auraNumber;
    }
}

void renderPyramidHeadSystem(Shader &lightingShader)
{
    if (!pyramidHeads.loaded)
        return;
    const glm::vec3 center = (pyramidHeads.aabbMin + pyramidHeads.aabbMax) * 0.5f;
    for (PyramidHeadEnemy &enemy : pyramidHeads.enemies)
    {
        if (!enemy.alive || glm::length(enemy.position - camera.Position) > renderDistance)
            continue;
        glm::mat4 model(1.0f);
        // The animated mesh origin sits below the visible feet. Raise Pyramid
        // Head so the character stands on, rather than inside, the road.
        model = glm::translate(model, enemy.position + glm::vec3(0.0f, 1.05f, 0.0f));
        model = glm::rotate(model, glm::radians(enemy.yaw), glm::vec3(0.0f, 1.0f, 0.0f));
        model = glm::scale(model, glm::vec3(pyramidHeads.renderScale));
        model = glm::translate(model, glm::vec3(-center.x, -pyramidHeads.aabbMin.y, -center.z));
        lightingShader.setMat4("model", model);
        lightingShader.setInt("useSkinning", pyramidHeads.boneCount > 0 ? 1 : 0);
        for (int i = 0; i < glm::min(pyramidHeads.boneCount, MAX_BONES); ++i)
            lightingShader.setMat4("finalBonesMatrices[" + std::to_string(i) + "]", enemy.animation.finalMatrices[i]);
        for (MeshData &mesh : pyramidHeads.meshes)
        {
            // The exporter renamed the arm/great-knife material to
            // Model001_Material001. It is the first mesh in this FBX.
            const bool armAndKnifeMesh = &mesh == &pyramidHeads.meshes.front();
            lightingShader.setInt("rigidSkinning", armAndKnifeMesh ? 1 : 0);
            lightingShader.setVec3("objectColor", mesh.materialColor);
            lightingShader.setFloat("objectAlpha", mesh.materialAlpha);
            lightingShader.setInt("useTextureAlpha", mesh.useTextureAlpha ? 1 : 0);
            lightingShader.setInt("useWhiteChromaKey", 0);
            lightingShader.setFloat("alphaCutoff", 0.01f);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, mesh.texture);
            glBindVertexArray(mesh.VAO);
            glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(mesh.indexCount), GL_UNSIGNED_INT, 0);
        }
    }
    lightingShader.setInt("useSkinning", 0);
    lightingShader.setInt("rigidSkinning", 0);
    lightingShader.setVec3("objectColor", 1.0f, 1.0f, 1.0f);
    lightingShader.setFloat("objectAlpha", 1.0f);
    lightingShader.setInt("useTextureAlpha", 1);
    lightingShader.setFloat("alphaCutoff", 0.38f);
}

void shutdownPyramidHeadSystem()
{
    for (MeshData &mesh : pyramidHeads.meshes)
    {
        if (mesh.VAO) glDeleteVertexArrays(1, &mesh.VAO);
        if (mesh.VBO) glDeleteBuffers(1, &mesh.VBO);
        if (mesh.EBO) glDeleteBuffers(1, &mesh.EBO);
        if (mesh.texture) glDeleteTextures(1, &mesh.texture);
    }
    pyramidHeads.meshes.clear();
}
