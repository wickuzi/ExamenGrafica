MeshData processMesh(aiMesh *mesh, const aiScene *scene, const glm::mat4 &transform, const std::string &directory, glm::vec3 &aabbMin, glm::vec3 &aabbMax, std::map<std::string, unsigned int> &loaded, std::unordered_map<std::string, BoneInfo> *boneInfoMap, int *boneCounter, bool renderable, bool collider)
{
    std::vector<float> vertices;
    std::vector<unsigned int> indices;
    MeshData mdata;
    mdata.renderable = renderable;
    mdata.collider = collider;
    glm::mat3 normalTransform = glm::transpose(glm::inverse(glm::mat3(transform)));
    std::vector<BoneVertexData> boneData(mesh->mNumVertices);
    for (auto &b : boneData)
    {
        b.ids.fill(-1);
        b.weights.fill(0.0f);
    }
    if (boneInfoMap && boneCounter && mesh->HasBones())
    {
        mdata.hasBones = true;
        for (unsigned int boneIndex = 0; boneIndex < mesh->mNumBones; ++boneIndex)
        {
            aiBone *bone = mesh->mBones[boneIndex];
            std::string boneName = bone->mName.C_Str();
            int boneID = -1;
            auto found = boneInfoMap->find(boneName);
            if (found == boneInfoMap->end())
            {
                if (*boneCounter >= MAX_BONES)
                    continue;
                BoneInfo info;
                info.id = *boneCounter;
                info.offset = aiToGlm(bone->mOffsetMatrix);
                (*boneInfoMap)[boneName] = info;
                boneID = *boneCounter;
                ++(*boneCounter);
            }
            else
            {
                boneID = found->second.id;
            }

            for (unsigned int weightIndex = 0; weightIndex < bone->mNumWeights; ++weightIndex)
            {
                unsigned int vertexId = bone->mWeights[weightIndex].mVertexId;
                float weight = bone->mWeights[weightIndex].mWeight;
                if (vertexId >= boneData.size())
                    continue;
                bool inserted = false;
                for (int slot = 0; slot < MAX_BONE_INFLUENCE; ++slot)
                {
                    if (boneData[vertexId].ids[slot] < 0)
                    {
                        boneData[vertexId].ids[slot] = boneID;
                        boneData[vertexId].weights[slot] = weight;
                        inserted = true;
                        break;
                    }
                }
                if (!inserted)
                {
                    int weakestSlot = 0;
                    for (int slot = 1; slot < MAX_BONE_INFLUENCE; ++slot)
                    {
                        if (boneData[vertexId].weights[slot] < boneData[vertexId].weights[weakestSlot])
                            weakestSlot = slot;
                    }
                    if (weight > boneData[vertexId].weights[weakestSlot])
                    {
                        boneData[vertexId].ids[weakestSlot] = boneID;
                        boneData[vertexId].weights[weakestSlot] = weight;
                    }
                }
            }
        }

        // A vertex can contain more influences than the four supported by the
        // shader. Renormalize the retained weights so discarded influences do
        // not collapse vertices toward the skeleton origin and create stretched
        // bright triangles across hands, elbows, or sleeves.
        for (auto &vertexBones : boneData)
        {
            float retainedWeight = 0.0f;
            for (float weight : vertexBones.weights)
                retainedWeight += weight;
            if (retainedWeight > 0.00001f)
            {
                for (float &weight : vertexBones.weights)
                    weight /= retainedWeight;
            }
        }
    }
    // vertices: pos(3), normal(3), tex(2)
    for (unsigned int i = 0; i < mesh->mNumVertices; ++i)
    {
        glm::mat4 vertexTransform = (boneInfoMap && boneCounter && mesh->HasBones()) ? glm::mat4(1.0f) : transform;
        glm::mat3 vertexNormalTransform = (boneInfoMap && boneCounter && mesh->HasBones()) ? glm::mat3(1.0f) : normalTransform;
        glm::vec3 pos = glm::vec3(vertexTransform * glm::vec4(mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z, 1.0f));
        vertices.push_back(pos.x);
        vertices.push_back(pos.y);
        vertices.push_back(pos.z);
        mdata.positions.push_back(pos);
        mdata.aabbMin.x = std::min(mdata.aabbMin.x, pos.x);
        mdata.aabbMin.y = std::min(mdata.aabbMin.y, pos.y);
        mdata.aabbMin.z = std::min(mdata.aabbMin.z, pos.z);
        mdata.aabbMax.x = std::max(mdata.aabbMax.x, pos.x);
        mdata.aabbMax.y = std::max(mdata.aabbMax.y, pos.y);
        mdata.aabbMax.z = std::max(mdata.aabbMax.z, pos.z);
        if (renderable)
        {
            aabbMin.x = std::min(aabbMin.x, pos.x);
            aabbMin.y = std::min(aabbMin.y, pos.y);
            aabbMin.z = std::min(aabbMin.z, pos.z);
            aabbMax.x = std::max(aabbMax.x, pos.x);
            aabbMax.y = std::max(aabbMax.y, pos.y);
            aabbMax.z = std::max(aabbMax.z, pos.z);
        }
        // normal
        if (mesh->HasNormals())
        {
            glm::vec3 normal = glm::normalize(vertexNormalTransform * glm::vec3(mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z));
            vertices.push_back(normal.x);
            vertices.push_back(normal.y);
            vertices.push_back(normal.z);
        }
        else
        {
            vertices.push_back(0.0f);
            vertices.push_back(0.0f);
            vertices.push_back(0.0f);
        }
        // texcoords
        if (mesh->mTextureCoords[0])
        {
            vertices.push_back(mesh->mTextureCoords[0][i].x);
            vertices.push_back(mesh->mTextureCoords[0][i].y);
        }
        else
        {
            vertices.push_back(0.0f);
            vertices.push_back(0.0f);
        }
        for (int slot = 0; slot < MAX_BONE_INFLUENCE; ++slot)
        {
            vertices.push_back(static_cast<float>(boneData[i].ids[slot]));
        }
        for (int slot = 0; slot < MAX_BONE_INFLUENCE; ++slot)
        {
            vertices.push_back(boneData[i].weights[slot]);
        }
    }
    for (unsigned int i = 0; i < mesh->mNumFaces; ++i)
    {
        aiFace face = mesh->mFaces[i];
        for (unsigned int j = 0; j < face.mNumIndices; ++j)
        {
            indices.push_back(face.mIndices[j]);
            mdata.indices.push_back(face.mIndices[j]);
        }
    }

    mdata.indexCount = (unsigned int)indices.size();

    glGenVertexArrays(1, &mdata.VAO);
    glGenBuffers(1, &mdata.VBO);
    glGenBuffers(1, &mdata.EBO);

    glBindVertexArray(mdata.VAO);
    glBindBuffer(GL_ARRAY_BUFFER, mdata.VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mdata.EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

    const GLsizei vertexStride = (8 + MAX_BONE_INFLUENCE + MAX_BONE_INFLUENCE) * sizeof(float);
    // pos
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, vertexStride, (void *)0);
    glEnableVertexAttribArray(0);
    // normal
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, vertexStride, (void *)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    // tex
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, vertexStride, (void *)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, vertexStride, (void *)(8 * sizeof(float)));
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, vertexStride, (void *)((8 + MAX_BONE_INFLUENCE) * sizeof(float)));
    glEnableVertexAttribArray(4);

    glBindVertexArray(0);

    // load material textures (diffuse), including embedded textures in glTF/.glb
    unsigned int texid = 0;
    if (mesh->mMaterialIndex >= 0)
    {
        aiMaterial *material = scene->mMaterials[mesh->mMaterialIndex];
        aiColor4D diffuseColor(1.0f, 1.0f, 1.0f, 1.0f);
        if (AI_SUCCESS == aiGetMaterialColor(material, AI_MATKEY_COLOR_DIFFUSE, &diffuseColor))
        {
            mdata.materialColor = glm::vec3(diffuseColor.r, diffuseColor.g, diffuseColor.b);
            mdata.materialAlpha = diffuseColor.a;
        }
        float opacity = 1.0f;
        if (AI_SUCCESS == material->Get(AI_MATKEY_OPACITY, opacity))
        {
            mdata.materialAlpha *= opacity;
        }
        aiString alphaMode;
        if (AI_SUCCESS == material->Get(AI_MATKEY_GLTF_ALPHAMODE, alphaMode))
        {
            std::string mode = alphaMode.C_Str();
            std::transform(mode.begin(), mode.end(), mode.begin(), ::toupper);
            mdata.useTextureAlpha = mode == "MASK" || mode == "BLEND";
        }
        else
        {
            std::string materialName = material->GetName().C_Str();
            std::transform(materialName.begin(), materialName.end(), materialName.begin(), ::tolower);
            mdata.useTextureAlpha = mdata.materialAlpha < 0.99f || materialName.find("alpha") != std::string::npos || materialName.find("glass") != std::string::npos;
        }
        // Some Assimp builds do not expose glTF alphaMode consistently. These
        // authored city materials are known atlas/cutout materials in the GLB.
        std::string alphaMaterialName = material->GetName().C_Str();
        std::transform(alphaMaterialName.begin(), alphaMaterialName.end(), alphaMaterialName.begin(), ::tolower);
        mdata.useTextureAlpha = mdata.useTextureAlpha ||
                                alphaMaterialName.find("foliage") != std::string::npos ||
                                alphaMaterialName.find("decal") != std::string::npos ||
                                alphaMaterialName.find("street_assets") != std::string::npos ||
                                alphaMaterialName.find("doors") != std::string::npos ||
                                alphaMaterialName.find("lanes_secondary") != std::string::npos;
        // These exported atlases contain nearly opaque white padding instead
        // of a usable transparent background. Remove that padding in shader
        // without affecting ordinary textures or James' materials.
        mdata.useWhiteChromaKey = alphaMaterialName.find("foliage") != std::string::npos ||
                                  alphaMaterialName.find("street_assets") != std::string::npos ||
                                  alphaMaterialName.find("decal") != std::string::npos;
        mdata.isFoliage = alphaMaterialName.find("foliage") != std::string::npos;
        const bool isMapMesh = boneInfoMap == nullptr;
        texid = loadMaterialTexture(material, scene, directory, loaded, isMapMesh || allowSkinnedTextureSearch);
        if (isMapMesh && texid == 0)
        {
            texid = loadTextureByMaterialName(material->GetName().C_Str(), directory, loaded);
        }
        if (texid == 0 && boneInfoMap && !allowSkinnedTextureSearch && !jamesFallbackTextures.empty())
        {
            std::string materialName = material->GetName().C_Str();
            std::string lowerName = materialName;
            std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);
            auto namedFallback = jamesFallbackTexturesByName.find(lowerName);
            if (namedFallback != jamesFallbackTexturesByName.end())
            {
                texid = namedFallback->second;
            }
            else
            {
                texid = jamesFallbackTextures[mesh->mMaterialIndex % jamesFallbackTextures.size()];
            }
            std::cout << "James FBX material fallback meshMat=" << mesh->mMaterialIndex
                      << " name='" << materialName << "' color=("
                      << mdata.materialColor.r << ", " << mdata.materialColor.g << ", " << mdata.materialColor.b
                      << ") tex=" << texid << std::endl;
        }
    }
    mdata.hasTexture = texid != 0;
    mdata.texture = texid;
    if (mdata.texture == 0)
        mdata.texture = getWhiteTexture();
    return mdata;
}

glm::mat4 aiToGlm(const aiMatrix4x4 &from)
{
    return glm::mat4(
        from.a1, from.b1, from.c1, from.d1,
        from.a2, from.b2, from.c2, from.d2,
        from.a3, from.b3, from.c3, from.d3,
        from.a4, from.b4, from.c4, from.d4);
}

bool findNodeWorldTransform(aiNode *node, const std::string &targetName, const glm::mat4 &parentTransform, glm::mat4 &outTransform)
{
    if (!node)
        return false;

    glm::mat4 worldTransform = parentTransform * aiToGlm(node->mTransformation);
    std::string nodeName = node->mName.C_Str();
    std::transform(nodeName.begin(), nodeName.end(), nodeName.begin(), ::tolower);
    auto normalizeNodeName = [](std::string value)
    {
        std::transform(value.begin(), value.end(), value.begin(), ::tolower);
        value.erase(std::remove_if(value.begin(), value.end(), [](unsigned char ch)
                                   { return ch == '_' || ch == '-' || std::isspace(ch); }),
                    value.end());
        return value;
    };
    if (normalizeNodeName(nodeName) == normalizeNodeName(targetName))
    {
        outTransform = worldTransform;
        return true;
    }

    for (unsigned int i = 0; i < node->mNumChildren; ++i)
    {
        if (findNodeWorldTransform(node->mChildren[i], targetName, worldTransform, outTransform))
            return true;
    }
    return false;
}

bool loadNodeWorldTransform(const std::string &path, const std::string &nodeName, glm::mat4 &outTransform)
{
    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(path, 0);
    if (!scene || !scene->mRootNode)
    {
        std::cout << "Node transform load failed: " << path << " " << importer.GetErrorString() << std::endl;
        return false;
    }

    std::string targetName = nodeName;
    std::transform(targetName.begin(), targetName.end(), targetName.begin(), ::tolower);
    bool found = findNodeWorldTransform(scene->mRootNode, targetName, glm::mat4(1.0f), outTransform);
    if (!found)
        std::cout << "Node not found in map: " << nodeName << std::endl;
    return found;
}

static void collectNodeWorldTransforms(aiNode *node, const std::string &namePrefix, const glm::mat4 &parentTransform, std::vector<glm::mat4> &outTransforms)
{
    if (!node)
        return;
    glm::mat4 worldTransform = parentTransform * aiToGlm(node->mTransformation);
    std::string nodeName = node->mName.C_Str();
    std::transform(nodeName.begin(), nodeName.end(), nodeName.begin(), ::tolower);
    std::string prefix = namePrefix;
    std::transform(prefix.begin(), prefix.end(), prefix.begin(), ::tolower);
    nodeName.erase(std::remove_if(nodeName.begin(), nodeName.end(), [](unsigned char ch)
                                  { return ch == '_' || ch == '-' || std::isspace(ch); }),
                   nodeName.end());
    prefix.erase(std::remove_if(prefix.begin(), prefix.end(), [](unsigned char ch)
                                { return ch == '_' || ch == '-' || std::isspace(ch); }),
                 prefix.end());
    if (nodeName.rfind(prefix, 0) == 0)
        outTransforms.push_back(worldTransform);
    for (unsigned int i = 0; i < node->mNumChildren; ++i)
        collectNodeWorldTransforms(node->mChildren[i], namePrefix, worldTransform, outTransforms);
}

bool loadNodeWorldTransforms(const std::string &path, const std::string &namePrefix, std::vector<glm::mat4> &outTransforms)
{
    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(path, 0);
    if (!scene || !scene->mRootNode)
        return false;
    outTransforms.clear();
    collectNodeWorldTransforms(scene->mRootNode, namePrefix, glm::mat4(1.0f), outTransforms);
    std::cout << "Found " << outTransforms.size() << " node(s) named " << namePrefix << "*." << std::endl;
    return !outTransforms.empty();
}

void processNode(const aiScene *scene, aiNode *node, const glm::mat4 &parentTransform, const std::string &directory, std::vector<MeshData> &meshes, glm::vec3 &aabbMin, glm::vec3 &aabbMax, std::map<std::string, unsigned int> &loaded, std::unordered_map<std::string, BoneInfo> *boneInfoMap, int *boneCounter, bool insideLightPos)
{
    glm::mat4 nodeTransform = parentTransform * aiToGlm(node->mTransformation);
    std::string nodeName = node->mName.C_Str();
    std::string lowerNodeName = nodeName;
    std::transform(lowerNodeName.begin(), lowerNodeName.end(), lowerNodeName.begin(), ::tolower);
    bool isWalkAreaNode = !boneInfoMap && isWalkAreaName(nodeName);
    bool isWalkZoneNode = !boneInfoMap && isWalkZoneName(nodeName);
    bool isSpawnNode = !boneInfoMap && isSpawnMarkerName(nodeName);
    bool isSavePointNode = !boneInfoMap && lowerNodeName.rfind("savepoint", 0) == 0;
    bool isLightPosNode = !boneInfoMap &&
                          (insideLightPos || lowerNodeName.rfind("lightpos", 0) == 0 || isSavePointNode);
    // The exported scene consolidates its real artwork under barricade.055.
    // Bare Cube.* nodes have no material and are Blender helper/boundary volumes.
    bool isHelperCubeNode = !boneInfoMap && !isWalkAreaNode && !isSpawnNode &&
                            (lowerNodeName == "cube" || lowerNodeName.rfind("cube.", 0) == 0);
    bool isColliderNode = !boneInfoMap &&
                          !isWalkAreaNode &&
                          !isSpawnNode &&
                          (isHelperCubeNode ||
                           lowerNodeName.find("collider") != std::string::npos ||
                           lowerNodeName.find("collision") != std::string::npos ||
                           lowerNodeName.find("boundary") != std::string::npos ||
                           lowerNodeName.find("invisible") != std::string::npos);
    for (unsigned int i = 0; i < node->mNumMeshes; ++i)
    {
        aiMesh *mesh = scene->mMeshes[node->mMeshes[i]];
        bool isWalkAreaMesh = isWalkAreaNode || isWalkAreaName(mesh->mName.C_Str());
        bool isWalkZoneMesh = isWalkZoneNode || isWalkZoneName(mesh->mName.C_Str());
        bool isSpawnMesh = isSpawnNode || isSpawnMarkerName(mesh->mName.C_Str());
        std::string meshName = mesh->mName.C_Str();
        std::transform(meshName.begin(), meshName.end(), meshName.begin(), ::tolower);
        // Blender walk/collision volumes are exported as Cube, Cube.001, etc.
        // Filter by mesh identity too, independently of the parent node name.
        bool isHelperCubeMesh = !boneInfoMap &&
                                (meshName == "cube" || meshName.rfind("cube.", 0) == 0);
        bool isLightPosMesh = isLightPosNode || meshName.rfind("lightpos", 0) == 0 ||
                              meshName.rfind("savepoint", 0) == 0;
        MeshData meshData = processMesh(mesh, scene, nodeTransform, directory, aabbMin, aabbMax, loaded, boneInfoMap, boneCounter,
                                        !isColliderNode && !isWalkAreaMesh && !isSpawnMesh && !isLightPosMesh && !isHelperCubeMesh,
                                        isColliderNode || isHelperCubeMesh);
        meshData.walkArea = isWalkAreaMesh;
        meshData.walkZone = isWalkZoneMesh;
        meshes.push_back(meshData);
    }
    for (unsigned int i = 0; i < node->mNumChildren; ++i)
        processNode(scene, node->mChildren[i], nodeTransform, directory, meshes, aabbMin, aabbMax, loaded, boneInfoMap, boneCounter, isLightPosNode);
}

std::vector<MeshData> loadModel(const std::string &path, glm::vec3 &outAABBMin, glm::vec3 &outAABBMax, std::unordered_map<std::string, BoneInfo> *boneInfoMap, int *boneCounter, glm::mat4 *outGlobalInverse)
{
    Assimp::Importer importer;
    std::string ext = std::filesystem::path(path).extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    unsigned int importFlags = aiProcess_Triangulate |
                               aiProcess_GenSmoothNormals |
                               aiProcess_JoinIdenticalVertices |
                               aiProcess_ImproveCacheLocality;
    if (ext != ".dae" && ext != ".glb" && ext != ".gltf")
    {
        importFlags |= aiProcess_FlipUVs;
    }
    const aiScene *scene = importer.ReadFile(path, importFlags);
    std::vector<MeshData> meshes;
    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
    {
        std::cerr << "ERROR::ASSIMP::" << importer.GetErrorString() << std::endl;
        return meshes;
    }
    if (outGlobalInverse)
    {
        *outGlobalInverse = glm::inverse(aiToGlm(scene->mRootNode->mTransformation));
    }
    // get directory
    std::string directory = path;
    size_t pos = directory.find_last_of("/\\");
    if (pos != std::string::npos)
        directory = directory.substr(0, pos);
    else
        directory = ".";
    std::map<std::string, unsigned int> loaded;
    processNode(scene, scene->mRootNode, glm::mat4(1.0f), directory, meshes, outAABBMin, outAABBMax, loaded, boneInfoMap, boneCounter);
    int texturedMeshes = 0;
    for (const auto &mesh : meshes)
    {
        if (mesh.hasTexture)
            texturedMeshes++;
    }
    std::cout << "Model texture summary for " << path << ": texturedMeshes=" << texturedMeshes
              << "/" << meshes.size() << " uniqueTextureRequests=" << loaded.size() << std::endl;
    return meshes;
}

AnimNode readAnimHierarchy(aiNode *node)
{
    AnimNode data;
    data.name = node->mName.C_Str();
    data.transform = aiToGlm(node->mTransformation);
    data.children.reserve(node->mNumChildren);
    for (unsigned int i = 0; i < node->mNumChildren; ++i)
    {
        data.children.push_back(readAnimHierarchy(node->mChildren[i]));
    }
    return data;
}

AnimationClip loadAnimationClip(const std::string &path, const std::string &name)
{
    AnimationClip clip;
    clip.name = name;

    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_LimitBoneWeights);
    if (!scene || !scene->mRootNode || scene->mNumAnimations == 0)
    {
        std::cout << "Animation load failed: " << path << " " << importer.GetErrorString() << std::endl;
        return clip;
    }

    aiAnimation *anim = scene->mAnimations[0];
    clip.duration = static_cast<float>(anim->mDuration);
    clip.ticksPerSecond = anim->mTicksPerSecond != 0.0 ? static_cast<float>(anim->mTicksPerSecond) : 25.0f;
    clip.root = readAnimHierarchy(scene->mRootNode);

    for (unsigned int i = 0; i < anim->mNumChannels; ++i)
    {
        aiNodeAnim *channel = anim->mChannels[i];
        AnimChannel out;
        out.name = channel->mNodeName.C_Str();
        out.positions.reserve(channel->mNumPositionKeys);
        out.rotations.reserve(channel->mNumRotationKeys);
        out.scales.reserve(channel->mNumScalingKeys);

        for (unsigned int key = 0; key < channel->mNumPositionKeys; ++key)
        {
            aiVector3D v = channel->mPositionKeys[key].mValue;
            out.positions.push_back({glm::vec3(v.x, v.y, v.z), static_cast<float>(channel->mPositionKeys[key].mTime)});
        }
        for (unsigned int key = 0; key < channel->mNumRotationKeys; ++key)
        {
            aiQuaternion q = channel->mRotationKeys[key].mValue;
            out.rotations.push_back({glm::normalize(glm::quat(q.w, q.x, q.y, q.z)), static_cast<float>(channel->mRotationKeys[key].mTime)});
        }
        for (unsigned int key = 0; key < channel->mNumScalingKeys; ++key)
        {
            aiVector3D v = channel->mScalingKeys[key].mValue;
            out.scales.push_back({glm::vec3(v.x, v.y, v.z), static_cast<float>(channel->mScalingKeys[key].mTime)});
        }
        clip.channels[out.name] = out;
    }

    clip.valid = true;
    return clip;
}

template <typename KeyT>
int getKeyIndex(const std::vector<KeyT> &keys, float animationTime)
{
    for (int i = 0; i + 1 < static_cast<int>(keys.size()); ++i)
    {
        if (animationTime < keys[i + 1].timeStamp)
            return i;
    }
    return glm::max(0, static_cast<int>(keys.size()) - 2);
}

float getScaleFactor(float lastTime, float nextTime, float animationTime)
{
    float span = nextTime - lastTime;
    if (fabsf(span) < 0.00001f)
        return 0.0f;
    return glm::clamp((animationTime - lastTime) / span, 0.0f, 1.0f);
}

glm::mat4 interpolateChannelTransform(const AnimChannel &channel, float animationTime)
{
    glm::vec3 translation(0.0f);
    glm::quat rotation(1.0f, 0.0f, 0.0f, 0.0f);
    glm::vec3 scale(1.0f);

    if (channel.positions.size() == 1)
    {
        translation = channel.positions[0].position;
    }
    else if (channel.positions.size() > 1)
    {
        int index = getKeyIndex(channel.positions, animationTime);
        int next = index + 1;
        float factor = getScaleFactor(channel.positions[index].timeStamp, channel.positions[next].timeStamp, animationTime);
        translation = glm::mix(channel.positions[index].position, channel.positions[next].position, factor);
    }

    if (channel.rotations.size() == 1)
    {
        rotation = channel.rotations[0].orientation;
    }
    else if (channel.rotations.size() > 1)
    {
        int index = getKeyIndex(channel.rotations, animationTime);
        int next = index + 1;
        float factor = getScaleFactor(channel.rotations[index].timeStamp, channel.rotations[next].timeStamp, animationTime);
        rotation = glm::normalize(glm::slerp(channel.rotations[index].orientation, channel.rotations[next].orientation, factor));
    }

    if (channel.scales.size() == 1)
    {
        scale = channel.scales[0].scale;
    }
    else if (channel.scales.size() > 1)
    {
        int index = getKeyIndex(channel.scales, animationTime);
        int next = index + 1;
        float factor = getScaleFactor(channel.scales[index].timeStamp, channel.scales[next].timeStamp, animationTime);
        scale = glm::mix(channel.scales[index].scale, channel.scales[next].scale, factor);
    }

    return glm::translate(glm::mat4(1.0f), translation) * glm::toMat4(rotation) * glm::scale(glm::mat4(1.0f), scale);
}

glm::mat4 interpolateChannelTransformNoTranslation(const AnimChannel &channel, float animationTime)
{
    glm::mat4 transform = interpolateChannelTransform(channel, animationTime);
    transform[3] = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
    return transform;
}

bool shouldStripTranslation(const std::string &nodeName)
{
    std::string lower = nodeName;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
    return lower == "root" ||
           lower.find("armature") != std::string::npos ||
           lower.find("hips") != std::string::npos ||
           lower.find("pelvis") != std::string::npos ||
           lower.find("mixamorig:hips") != std::string::npos;
}

void calculateBoneTransforms(const AnimNode &node, const glm::mat4 &parentTransform, const AnimationClip &clip, std::vector<glm::mat4> &finalMatrices, const std::unordered_map<std::string, BoneInfo> &boneInfoMap)
{
    glm::mat4 nodeTransform = node.transform;
    auto channel = clip.channels.find(node.name);
    if (channel != clip.channels.end())
    {
        nodeTransform = interpolateChannelTransform(channel->second, clip.duration > 0.0f ? fmodf(clip.duration, clip.duration) : 0.0f);
    }

    glm::mat4 globalTransform = parentTransform * nodeTransform;
    auto bone = boneInfoMap.find(node.name);
    if (bone != boneInfoMap.end() && bone->second.id < MAX_BONES)
    {
        finalMatrices[bone->second.id] = globalTransform * bone->second.offset;
    }

    for (const auto &child : node.children)
    {
        calculateBoneTransforms(child, globalTransform, clip, finalMatrices, boneInfoMap);
    }
}

void calculateBoneTransformsAtTime(const AnimNode &node, const glm::mat4 &parentTransform, const AnimationClip &clip, float animationTime, std::vector<glm::mat4> &finalMatrices, const std::unordered_map<std::string, BoneInfo> &boneInfoMap, const glm::mat4 &globalInverseTransform, bool isRoot)
{
    glm::mat4 nodeTransform = node.transform;
    auto channel = clip.channels.find(node.name);
    if (channel != clip.channels.end())
    {
        nodeTransform = (isRoot || shouldStripTranslation(node.name))
                            ? interpolateChannelTransformNoTranslation(channel->second, animationTime)
                            : interpolateChannelTransform(channel->second, animationTime);
    }

    glm::mat4 globalTransform = parentTransform * nodeTransform;
    auto bone = boneInfoMap.find(node.name);
    if (bone != boneInfoMap.end() && bone->second.id < MAX_BONES)
    {
        finalMatrices[bone->second.id] = globalInverseTransform * globalTransform * bone->second.offset;
    }

    for (const auto &child : node.children)
    {
        calculateBoneTransformsAtTime(child, globalTransform, clip, animationTime, finalMatrices, boneInfoMap, globalInverseTransform, false);
    }
}

void updateAnimation(AnimationState &state, const AnimationClip *clip, float deltaSeconds, const std::unordered_map<std::string, BoneInfo> &boneInfoMap, int boneCount, const glm::mat4 &globalInverseTransform, bool looping)
{
    if (!clip || !clip->valid || boneCount <= 0)
    {
        state.finalMatrices.assign(MAX_BONES, glm::mat4(1.0f));
        state.current = nullptr;
        state.currentTime = 0.0f;
        return;
    }

    if (state.current != clip)
    {
        state.current = clip;
        state.currentTime = 0.0f;
        state.currentLooping = looping;
    }
    state.currentTime += clip->ticksPerSecond * deltaSeconds;
    if (clip->duration > 0.0f)
    {
        if (looping)
        {
            state.currentTime = fmodf(state.currentTime, clip->duration);
        }
        else
        {
            state.currentTime = glm::min(state.currentTime, glm::max(0.0f, clip->duration - 1.0f));
        }
    }

    state.finalMatrices.assign(MAX_BONES, glm::mat4(1.0f));
    calculateBoneTransformsAtTime(clip->root, glm::mat4(1.0f), *clip, state.currentTime, state.finalMatrices, boneInfoMap, globalInverseTransform, true);
}

const AnimationClip *findClip(const std::unordered_map<std::string, AnimationClip> &clips, const std::string &name)
{
    auto it = clips.find(name);
    return it == clips.end() ? nullptr : &it->second;
}
