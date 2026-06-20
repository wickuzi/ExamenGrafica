bool isWalkAreaName(const std::string &name)
{
    std::string lowerName = name;
    std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(),
                   [](unsigned char ch)
                   { return static_cast<char>(std::tolower(ch)); });
    return lowerName.find("walkarea") != std::string::npos || isWalkZoneName(name);
}

bool isWalkZoneName(const std::string &name)
{
    std::string lowerName = name;
    std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(),
                   [](unsigned char ch)
                   { return static_cast<char>(std::tolower(ch)); });
    return lowerName.find("walkzone") != std::string::npos;
}

bool isSpawnMarkerName(const std::string &name)
{
    std::string lowerName = name;
    std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(),
                   [](unsigned char ch)
                   { return static_cast<char>(std::tolower(ch)); });
    lowerName.erase(std::remove_if(lowerName.begin(), lowerName.end(), [](unsigned char ch)
                                   { return ch == '_' || ch == '-' || std::isspace(ch); }),
                    lowerName.end());
    return lowerName == "spawnplayer";
}

std::vector<WalkTriangle> buildWalkTriangles(const std::vector<MeshData> &meshes, const glm::mat4 &modelTransform)
{
    std::vector<WalkTriangle> triangles;
    for (const auto &mesh : meshes)
    {
        if (mesh.collider || !mesh.renderable)
            continue;
        for (size_t i = 0; i + 2 < mesh.indices.size(); i += 3)
        {
            glm::vec3 a = glm::vec3(modelTransform * glm::vec4(mesh.positions[mesh.indices[i]], 1.0f));
            glm::vec3 b = glm::vec3(modelTransform * glm::vec4(mesh.positions[mesh.indices[i + 1]], 1.0f));
            glm::vec3 c = glm::vec3(modelTransform * glm::vec4(mesh.positions[mesh.indices[i + 2]], 1.0f));
            glm::vec3 normal = glm::cross(b - a, c - a);
            float area = glm::length(normal);
            if (area < 0.0001f)
                continue;
            normal = glm::normalize(normal);
            if (fabsf(normal.y) < 0.45f)
                continue;

            WalkTriangle tri{};
            tri.a = a;
            tri.b = b;
            tri.c = c;
            tri.minX = glm::min(a.x, glm::min(b.x, c.x));
            tri.maxX = glm::max(a.x, glm::max(b.x, c.x));
            tri.minZ = glm::min(a.z, glm::min(b.z, c.z));
            tri.maxZ = glm::max(a.z, glm::max(b.z, c.z));
            tri.area = fabsf((b.x - a.x) * (c.z - a.z) - (c.x - a.x) * (b.z - a.z)) * 0.5f;
            triangles.push_back(tri);
        }
    }
    return triangles;
}

std::vector<WalkArea> buildWalkAreas(const std::vector<MeshData> &meshes, const glm::mat4 &modelTransform)
{
    std::vector<WalkArea> areas;
    bool hasAuthoredWalkZones = std::any_of(meshes.begin(), meshes.end(), [](const MeshData &mesh)
                                            { return mesh.walkZone; });
    for (const auto &mesh : meshes)
    {
        if (!mesh.walkArea || mesh.positions.empty() || (hasAuthoredWalkZones && !mesh.walkZone))
            continue;
        for (size_t i = 0; i + 2 < mesh.indices.size(); i += 3)
        {
            glm::vec3 a = glm::vec3(modelTransform * glm::vec4(mesh.positions[mesh.indices[i]], 1.0f));
            glm::vec3 b = glm::vec3(modelTransform * glm::vec4(mesh.positions[mesh.indices[i + 1]], 1.0f));
            glm::vec3 c = glm::vec3(modelTransform * glm::vec4(mesh.positions[mesh.indices[i + 2]], 1.0f));
            glm::vec3 crossValue = glm::cross(b - a, c - a);
            float projectedArea = fabsf((b.x - a.x) * (c.z - a.z) - (c.x - a.x) * (b.z - a.z));
            if (glm::length(crossValue) < 0.0001f || projectedArea < 0.0001f ||
                fabsf(glm::normalize(crossValue).y) < 0.60f)
                continue;

            WalkArea area{};
            area.a = a;
            area.b = b;
            area.c = c;
            area.minX = glm::min(a.x, glm::min(b.x, c.x));
            area.maxX = glm::max(a.x, glm::max(b.x, c.x));
            area.floorY = (a.y + b.y + c.y) / 3.0f;
            area.minZ = glm::min(a.z, glm::min(b.z, c.z));
            area.maxZ = glm::max(a.z, glm::max(b.z, c.z));
            areas.push_back(area);
        }
    }
    std::cout << "Loaded " << areas.size() << " exact walk triangles from "
              << (hasAuthoredWalkZones ? "walkzone" : "walkarea") << " nodes." << std::endl;
    return areas;
}

static bool pointInWalkTriangle(float x, float z, const WalkArea &area)
{
    glm::vec2 p(x, z), a(area.a.x, area.a.z), b(area.b.x, area.b.z), c(area.c.x, area.c.z);
    float denominator = (b.y - c.y) * (a.x - c.x) + (c.x - b.x) * (a.y - c.y);
    if (fabsf(denominator) < 0.00001f)
        return false;
    float u = ((b.y - c.y) * (p.x - c.x) + (c.x - b.x) * (p.y - c.y)) / denominator;
    float v = ((c.y - a.y) * (p.x - c.x) + (a.x - c.x) * (p.y - c.y)) / denominator;
    float w = 1.0f - u - v;
    const float edgeTolerance = -0.015f;
    return u >= edgeTolerance && v >= edgeTolerance && w >= edgeTolerance;
}

bool canWalkHere(float x, float z)
{
    if (walkAreas.empty())
        return true;

    for (const auto &area : walkAreas)
    {
        if (x >= area.minX && x <= area.maxX && z >= area.minZ && z <= area.maxZ &&
            pointInWalkTriangle(x, z, area))
        {
            return true;
        }
    }
    return false;
}

bool findWalkAreaHeightAt(float x, float z, float &outY)
{
    bool found = false;
    float bestY = -FLT_MAX;
    for (const auto &area : walkAreas)
    {
        if (x >= area.minX && x <= area.maxX && z >= area.minZ && z <= area.maxZ &&
            pointInWalkTriangle(x, z, area) &&
            area.floorY > bestY)
        {
            bestY = area.floorY;
            found = true;
        }
    }

    if (found)
        outY = bestY;
    return found;
}

glm::vec3 findWalkAreaSpawnPoint()
{
    const WalkArea &area = walkAreas.front();
    glm::vec3 spawn(
        (area.minX + area.maxX) * 0.5f,
        area.floorY + JAMES_HEIGHT_OFFSET,
        (area.minZ + area.maxZ) * 0.5f);
    std::cout << "spawn_player node not found. Using first walkarea center at ("
              << spawn.x << ", " << spawn.y << ", " << spawn.z << ")" << std::endl;
    return spawn;
}

void updateWalkBoundary()
{
    if (walkTriangles.empty())
    {
        hasWalkBoundary = false;
        return;
    }

    walkBoundaryMin = glm::vec2(FLT_MAX);
    walkBoundaryMax = glm::vec2(-FLT_MAX);
    for (const auto &tri : walkTriangles)
    {
        walkBoundaryMin.x = glm::min(walkBoundaryMin.x, tri.minX);
        walkBoundaryMin.y = glm::min(walkBoundaryMin.y, tri.minZ);
        walkBoundaryMax.x = glm::max(walkBoundaryMax.x, tri.maxX);
        walkBoundaryMax.y = glm::max(walkBoundaryMax.y, tri.maxZ);
    }
    hasWalkBoundary = true;
    std::cout << "Walk boundary x=(" << walkBoundaryMin.x << ", " << walkBoundaryMax.x
              << ") z=(" << walkBoundaryMin.y << ", " << walkBoundaryMax.y << ")" << std::endl;
}

bool isBlockedByCollisionBoxes(const glm::vec3 &position)
{
    if (hasWalkBoundary)
    {
        const float margin = 0.55f;
        if (position.x < walkBoundaryMin.x + margin || position.x > walkBoundaryMax.x - margin ||
            position.z < walkBoundaryMin.y + margin || position.z > walkBoundaryMax.y - margin)
        {
            return true;
        }
    }

    const float playerRadius = 0.34f;
    const float playerHeight = 1.65f;
    for (const auto &box : collisionBoxes)
    {
        if (position.y + playerHeight < box.min.y || position.y > box.max.y)
            continue;
        if (position.x >= box.min.x - playerRadius && position.x <= box.max.x + playerRadius &&
            position.z >= box.min.z - playerRadius && position.z <= box.max.z + playerRadius)
        {
            return true;
        }
    }
    return false;
}

glm::vec3 findSpawnPoint()
{
    if (walkTriangles.empty())
        return glm::vec3(0.0f);

    float lowestY = FLT_MAX;
    for (const auto &tri : walkTriangles)
    {
        glm::vec3 center = (tri.a + tri.b + tri.c) / 3.0f;
        lowestY = glm::min(lowestY, center.y);
    }

    const WalkTriangle *best = nullptr;
    float bestScore = -FLT_MAX;
    for (const auto &tri : walkTriangles)
    {
        glm::vec3 center = (tri.a + tri.b + tri.c) / 3.0f;
        if (center.y > lowestY + 1.2f)
            continue;
        float distanceFromOrigin = glm::length(glm::vec2(center.x, center.z));
        float score = tri.area - distanceFromOrigin * 0.05f;
        if (score > bestScore)
        {
            bestScore = score;
            best = &tri;
        }
    }

    if (!best)
    {
        for (const auto &tri : walkTriangles)
        {
            glm::vec3 center = (tri.a + tri.b + tri.c) / 3.0f;
            float score = -center.y;
            if (score > bestScore)
            {
                bestScore = score;
                best = &tri;
            }
        }
    }

    glm::vec3 spawn = (best->a + best->b + best->c) / 3.0f;
    spawn.y += 0.02f;
    return spawn;
}

glm::vec3 findTownVisualSpawnPoint()
{
    if (walkTriangles.empty())
        return glm::vec3(0.0f);

    float lowestY = FLT_MAX;
    float minZ = FLT_MAX;
    float maxZ = -FLT_MAX;
    for (const auto &tri : walkTriangles)
    {
        glm::vec3 center = (tri.a + tri.b + tri.c) / 3.0f;
        lowestY = glm::min(lowestY, center.y);
        minZ = glm::min(minZ, center.z);
        maxZ = glm::max(maxZ, center.z);
    }

    float preferredZ = maxZ - (maxZ - minZ) * 0.18f;
    const WalkTriangle *best = nullptr;
    float bestScore = -FLT_MAX;
    for (const auto &tri : walkTriangles)
    {
        glm::vec3 center = (tri.a + tri.b + tri.c) / 3.0f;
        if (center.y > lowestY + 2.2f || tri.area < 0.30f)
            continue;

        float zScore = -fabsf(center.z - preferredZ) * 0.55f;
        float xScore = -fabsf(center.x) * 0.12f;
        float areaScore = glm::min(tri.area, 14.0f) * 0.75f;
        float score = zScore + xScore + areaScore;
        if (score > bestScore)
        {
            bestScore = score;
            best = &tri;
        }
    }

    if (!best)
        return findSpawnPoint();

    glm::vec3 spawn = (best->a + best->b + best->c) / 3.0f;
    spawn.y += 0.02f;
    std::cout << "Town visual spawn selected at (" << spawn.x << ", " << spawn.y << ", " << spawn.z
              << ") zRange=(" << minZ << ", " << maxZ << ") preferredZ=" << preferredZ << std::endl;
    return spawn;
}

bool findGroundHeightAt(float x, float z, float maxStepUp, float &outY)
{
    float bestY = -FLT_MAX;
    for (const auto &tri : walkTriangles)
    {
        if (x < tri.minX || x > tri.maxX || z < tri.minZ || z > tri.maxZ)
            continue;

        glm::vec2 a(tri.a.x, tri.a.z);
        glm::vec2 b(tri.b.x, tri.b.z);
        glm::vec2 c(tri.c.x, tri.c.z);
        glm::vec2 p(x, z);
        glm::vec2 v0 = b - a;
        glm::vec2 v1 = c - a;
        glm::vec2 v2 = p - a;
        float den = v0.x * v1.y - v1.x * v0.y;
        if (fabsf(den) < 0.000001f)
            continue;

        float u = (v2.x * v1.y - v1.x * v2.y) / den;
        float v = (v0.x * v2.y - v2.x * v0.y) / den;
        float w = 1.0f - u - v;
        if (u >= -0.001f && v >= -0.001f && w >= -0.001f)
        {
            float y = w * tri.a.y + u * tri.b.y + v * tri.c.y;
            if (y > bestY && y <= maxStepUp)
            {
                bestY = y;
            }
        }
    }
    if (bestY > -FLT_MAX * 0.5f)
    {
        outY = bestY;
        return true;
    }
    return false;
}

float findGroundHeight(float x, float z, float fallbackY)
{
    float y = fallbackY;
    return findGroundHeightAt(x, z, fallbackY + 2.0f, y) ? y : fallbackY;
}

bool findAnyGroundHeightAt(float x, float z, float &outY)
{
    return findGroundHeightAt(x, z, FLT_MAX, outY);
}

glm::vec3 closestPointOnTriangle(const glm::vec3 &p, const glm::vec3 &a, const glm::vec3 &b, const glm::vec3 &c)
{
    glm::vec3 ab = b - a;
    glm::vec3 ac = c - a;
    glm::vec3 ap = p - a;
    float d1 = glm::dot(ab, ap);
    float d2 = glm::dot(ac, ap);
    if (d1 <= 0.0f && d2 <= 0.0f)
        return a;

    glm::vec3 bp = p - b;
    float d3 = glm::dot(ab, bp);
    float d4 = glm::dot(ac, bp);
    if (d3 >= 0.0f && d4 <= d3)
        return b;

    float vc = d1 * d4 - d3 * d2;
    if (vc <= 0.0f && d1 >= 0.0f && d3 <= 0.0f)
    {
        float v = d1 / (d1 - d3);
        return a + v * ab;
    }

    glm::vec3 cp = p - c;
    float d5 = glm::dot(ab, cp);
    float d6 = glm::dot(ac, cp);
    if (d6 >= 0.0f && d5 <= d6)
        return c;

    float vb = d5 * d2 - d1 * d6;
    if (vb <= 0.0f && d2 >= 0.0f && d6 <= 0.0f)
    {
        float w = d2 / (d2 - d6);
        return a + w * ac;
    }

    float va = d3 * d6 - d5 * d4;
    if (va <= 0.0f && (d4 - d3) >= 0.0f && (d5 - d6) >= 0.0f)
    {
        float w = (d4 - d3) / ((d4 - d3) + (d5 - d6));
        return b + w * (c - b);
    }

    float denom = 1.0f / (va + vb + vc);
    float v = vb * denom;
    float w = vc * denom;
    return a + ab * v + ac * w;
}

bool findWallAttachment(const std::vector<MeshData> &meshes, const glm::mat4 &modelTransform, const glm::vec3 &anchor, const glm::vec3 &preferredNormal, glm::vec3 &outPosition, glm::vec3 &outNormal)
{
    bool found = false;
    float bestScore = FLT_MAX;
    const float targetHeight = anchor.y;
    const float minWallHeight = 1.15f;
    glm::vec3 preferred = glm::normalize(preferredNormal);

    for (const auto &mesh : meshes)
    {
        for (size_t i = 0; i + 2 < mesh.indices.size(); i += 3)
        {
            glm::vec3 a = glm::vec3(modelTransform * glm::vec4(mesh.positions[mesh.indices[i]], 1.0f));
            glm::vec3 b = glm::vec3(modelTransform * glm::vec4(mesh.positions[mesh.indices[i + 1]], 1.0f));
            glm::vec3 c = glm::vec3(modelTransform * glm::vec4(mesh.positions[mesh.indices[i + 2]], 1.0f));
            glm::vec3 normal = glm::cross(b - a, c - a);
            float doubleArea = glm::length(normal);
            if (doubleArea < 0.12f)
                continue;

            normal = glm::normalize(normal);
            if (fabsf(normal.y) > 0.25f)
                continue;

            glm::vec3 closest = closestPointOnTriangle(anchor, a, b, c);
            if (closest.y < minWallHeight)
                continue;

            float distance = glm::length(closest - anchor);
            if (distance > 6.0f)
                continue;

            float heightError = fabsf(closest.y - targetHeight);
            glm::vec3 outwardNormal = glm::dot(normal, preferred) < 0.0f ? -normal : normal;
            float normalMismatch = 1.0f - glm::max(glm::dot(outwardNormal, preferred), 0.0f);
            if (normalMismatch > 0.55f)
                continue;

            float score = distance + heightError * 4.0f + normalMismatch * 8.0f - glm::min(doubleArea, 3.0f) * 0.08f;
            if (score < bestScore)
            {
                bestScore = score;
                outNormal = outwardNormal;
                outPosition = closest + outNormal * 0.018f;
                found = true;
            }
        }
    }

    return found;
}

glm::mat4 makeWallModel(const glm::vec3 &position, const glm::vec3 &normal, const glm::vec3 &scale)
{
    glm::vec3 n = glm::normalize(normal);
    glm::vec3 up(0.0f, 1.0f, 0.0f);
    glm::vec3 right = glm::normalize(glm::cross(up, n));
    if (glm::length(right) < 0.001f)
        right = glm::vec3(1.0f, 0.0f, 0.0f);
    glm::vec3 localUp = glm::normalize(glm::cross(n, right));

    glm::mat4 basis(1.0f);
    basis[0] = glm::vec4(right, 0.0f);
    basis[1] = glm::vec4(localUp, 0.0f);
    basis[2] = glm::vec4(n, 0.0f);

    return glm::translate(glm::mat4(1.0f), position) * basis * glm::scale(glm::mat4(1.0f), scale);
}
