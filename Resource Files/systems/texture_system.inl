unsigned int loadTextureFromFile(const std::string &path, const std::string &directory, std::map<std::string, unsigned int> &loaded, bool allowGlobalTextureSearch)
{
    std::string filename = path;
    // if path is relative, prepend directory
    std::filesystem::path inputPath(filename);
    if (filename.size() && !inputPath.is_absolute() && filename.find(":/") == std::string::npos && filename.find(":\\") == std::string::npos && (filename.find("\\") != 0))
    {
        filename = directory + "/" + filename;
    }
    // if the file does not exist (e.g. absolute path to another machine), try to locate it in local assets
    if (!std::filesystem::exists(filename))
    {
        std::cout << "Texture file not found at original path: " << filename << ". Will try to locate in assets/" << std::endl;
        auto base = std::filesystem::path(filename).filename().string();
        std::filesystem::path candidate1 = std::filesystem::path("assets") / "textures" / base;
        std::filesystem::path candidate2 = std::filesystem::path("assets") / base;
        bool found = false;
        if (std::filesystem::exists(candidate1))
        {
            filename = candidate1.string();
            std::cout << "Found texture in: " << filename << std::endl;
            found = true;
        }
        else if (std::filesystem::exists(candidate2))
        {
            filename = candidate2.string();
            std::cout << "Found texture in: " << filename << std::endl;
            found = true;
        }
        else if (allowGlobalTextureSearch)
        {
            std::filesystem::path originalTexturePath(path);
            std::string parentHint = originalTexturePath.parent_path().filename().string();
            std::transform(parentHint.begin(), parentHint.end(), parentHint.begin(), ::tolower);

            std::vector<std::string> baseVariants = {base};
            std::string stem = std::filesystem::path(base).stem().string();
            std::string ext = std::filesystem::path(base).extension().string();
            std::string lowerStemForVariant = stem;
            std::transform(lowerStemForVariant.begin(), lowerStemForVariant.end(), lowerStemForVariant.begin(), ::tolower);
            const std::string imageSuffix = "_001";
            if (lowerStemForVariant.size() > imageSuffix.size() &&
                lowerStemForVariant.rfind(imageSuffix) == lowerStemForVariant.size() - imageSuffix.size())
            {
                baseVariants.push_back(stem.substr(0, stem.size() - imageSuffix.size()) + ext);
            }

            std::vector<std::filesystem::path> mapTextureRoots = {
                std::filesystem::path(directory) / "town_visual_textures",
                std::filesystem::path("models") / "town_visual_textures",
                std::filesystem::path("..") / "models" / "town_visual_textures"};
            std::vector<std::string> extensionVariants = {"", ".png", ".jpg", ".jpeg", ".tga"};
            for (const auto &root : mapTextureRoots)
            {
                if (found || parentHint.empty())
                    break;
                for (const auto &baseVariant : baseVariants)
                {
                    if (found)
                        break;
                    for (const auto &extensionVariant : extensionVariants)
                    {
                        std::filesystem::path candidate = root / parentHint / (baseVariant + extensionVariant);
                        if (std::filesystem::exists(candidate))
                        {
                            filename = candidate.string();
                            std::cout << "Found texture by original folder hint: " << filename << std::endl;
                            found = true;
                            break;
                        }
                    }
                }
            }
            for (const auto &root : mapTextureRoots)
            {
                if (found)
                    break;
                for (const auto &baseVariant : baseVariants)
                {
                    if (found)
                        break;
                    for (const auto &extensionVariant : extensionVariants)
                    {
                        std::filesystem::path candidate = root / "embedded" / (baseVariant + extensionVariant);
                        if (std::filesystem::exists(candidate))
                        {
                            filename = candidate.string();
                            std::cout << "Found texture in town_visual embedded cache: " << filename << std::endl;
                            found = true;
                            break;
                        }
                    }
                }
            }

            std::vector<std::filesystem::path> searchRoots;
            searchRoots.push_back(std::filesystem::path(directory));
            if (std::filesystem::path(directory).has_parent_path())
                searchRoots.push_back(std::filesystem::path(directory).parent_path());
            searchRoots.push_back(std::filesystem::path("models"));
            searchRoots.push_back(std::filesystem::path("..") / "models");
            searchRoots.push_back(std::filesystem::path("assets"));
            searchRoots.push_back(std::filesystem::path("..") / "assets");
            std::string lowerBase = base;
            std::transform(lowerBase.begin(), lowerBase.end(), lowerBase.begin(), ::tolower);
            std::string lowerStem = std::filesystem::path(base).stem().string();
            std::transform(lowerStem.begin(), lowerStem.end(), lowerStem.begin(), ::tolower);
            std::vector<std::string> numericStemCandidates;
            std::string numericRun;
            for (char ch : lowerStem)
            {
                if (std::isdigit(static_cast<unsigned char>(ch)))
                {
                    numericRun.push_back(ch);
                }
                else if (!numericRun.empty())
                {
                    numericStemCandidates.push_back(numericRun);
                    numericRun.clear();
                }
            }
            if (!numericRun.empty())
                numericStemCandidates.push_back(numericRun);
            for (auto &root : searchRoots)
            {
                if (found)
                    break;
                try
                {
                    if (!std::filesystem::exists(root))
                        continue;
                    for (auto &p : std::filesystem::recursive_directory_iterator(root))
                    {
                        if (!p.is_regular_file())
                            continue;
                        std::string candidateName = p.path().filename().string();
                        std::transform(candidateName.begin(), candidateName.end(), candidateName.begin(), ::tolower);
                        std::string candidateStem = p.path().stem().string();
                        std::transform(candidateStem.begin(), candidateStem.end(), candidateStem.begin(), ::tolower);
                        bool numericMatch = false;
                        for (const auto &numericCandidate : numericStemCandidates)
                        {
                            if (candidateStem == numericCandidate)
                            {
                                numericMatch = true;
                                break;
                            }
                        }
                        if (candidateName == lowerBase || candidateStem == lowerStem || numericMatch)
                        {
                            filename = p.path().string();
                            std::cout << "Found texture by recursive search: " << filename << std::endl;
                            found = true;
                            break;
                        }
                    }
                }
                catch (std::filesystem::filesystem_error &e)
                {
                    std::cout << "Filesystem error while searching " << root.string() << ": " << e.what() << std::endl;
                }
                if (found)
                    break;
            }
            if (!found)
                std::cout << "Could not find texture '" << base << "' in assets/; using empty texture" << std::endl;
        }
        else
        {
            std::cout << "Texture global search disabled for '" << base << "'; using model-local texture fallback" << std::endl;
        }
    }
    if (loaded.find(filename) != loaded.end())
        return loaded[filename];
    size_t extPos = filename.find_last_of('.');
    std::string ext = extPos != std::string::npos ? filename.substr(extPos + 1) : std::string();
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

    unsigned int tex = 0;
    if (ext != "tga" && ext != "ktx" && ext != "dds")
    {
        std::wstring wfn(filename.begin(), filename.end());
        tex = loadTextureFromJpeg(wfn.c_str());
    }
    if (tex == 0)
    {
        if (ext == "tga" || ext == "ktx" || ext == "dds")
        {
            stbi_set_flip_vertically_on_load(1);
            int w = 0, h = 0, n = 0;
            unsigned char *data = stbi_load(filename.c_str(), &w, &h, &n, 4);
            if (data)
            {
                glGenTextures(1, &tex);
                glBindTexture(GL_TEXTURE_2D, tex);
                glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
                applyTextureParams(true);
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
                glGenerateMipmap(GL_TEXTURE_2D);
                stbi_image_free(data);
                std::cout << "Loaded texture via stb_image: " << filename << std::endl;
            }
            else
            {
                std::cout << "stb_image failed to load: " << filename << " (" << stbi_failure_reason() << ")" << std::endl;
            }
        }
        if (tex == 0)
            std::cout << "loadTextureFromFile: failed to create GL texture for '" << filename << "'" << std::endl;
    }
    loaded[filename] = tex;
    return tex;
}

unsigned int loadTextureByMaterialName(const std::string &materialName, const std::string &directory, std::map<std::string, unsigned int> &loaded)
{
    std::string lowerName = materialName;
    std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);

    std::vector<std::string> candidates;
    std::string current;
    for (char ch : lowerName)
    {
        if (std::isdigit(static_cast<unsigned char>(ch)))
        {
            current.push_back(ch);
        }
        else if (!current.empty())
        {
            candidates.push_back(current);
            current.clear();
        }
    }
    if (!current.empty())
        candidates.push_back(current);
    if (lowerName.find("sh_initial_loader") != std::string::npos || lowerName.find("refrep") != std::string::npos)
        candidates.push_back("SH_initial_loader.exe_Sat_Dec_03_17-03-58_2005_1");

    std::vector<std::filesystem::path> roots = {
        std::filesystem::path(directory),
        std::filesystem::path(directory) / "town_visual_textures",
        std::filesystem::path("models") / "town_visual_textures",
        std::filesystem::path("..") / "models" / "town_visual_textures"};

    for (const auto &candidate : candidates)
    {
        for (const auto &root : roots)
        {
            if (!std::filesystem::exists(root))
                continue;
            try
            {
                for (const auto &entry : std::filesystem::recursive_directory_iterator(root))
                {
                    if (!entry.is_regular_file())
                        continue;
                    std::string stem = entry.path().stem().string();
                    std::transform(stem.begin(), stem.end(), stem.begin(), ::tolower);
                    std::string lowerCandidate = candidate;
                    std::transform(lowerCandidate.begin(), lowerCandidate.end(), lowerCandidate.begin(), ::tolower);
                    if (stem == lowerCandidate)
                    {
                        std::cout << "Material texture fallback: " << materialName << " -> " << entry.path().string() << std::endl;
                        return loadTextureFromFile(entry.path().string(), directory, loaded);
                    }
                }
            }
            catch (const std::filesystem::filesystem_error &e)
            {
                std::cout << "Material texture fallback search failed: " << e.what() << std::endl;
            }
        }
    }

    return 0;
}

unsigned int getWhiteTexture()
{
    static unsigned int whiteTex = 0;
    if (whiteTex)
        return whiteTex;
    unsigned char data[4] = {255, 255, 255, 255};
    glGenTextures(1, &whiteTex);
    glBindTexture(GL_TEXTURE_2D, whiteTex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    return whiteTex;
}

unsigned int loadEmbeddedTexture(aiTexture *atex, bool flipVertically = false, bool generateMipmaps = true)
{
    unsigned int glt = 0;
    if (atex->mHeight == 0)
    {
        int w = 0, h = 0, n = 0;
        stbi_set_flip_vertically_on_load(flipVertically ? 1 : 0);
        unsigned char *img = stbi_load_from_memory((unsigned char *)atex->pcData, (int)atex->mWidth, &w, &h, &n, 4);
        stbi_set_flip_vertically_on_load(0);
        if (img)
        {
            glGenTextures(1, &glt);
            glBindTexture(GL_TEXTURE_2D, glt);
            glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
            applyTextureParams(generateMipmaps);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, img);
            if (generateMipmaps)
                glGenerateMipmap(GL_TEXTURE_2D);
            stbi_image_free(img);
        }
    }
    else
    {
        int w = atex->mWidth;
        int h = atex->mHeight;
        std::vector<unsigned char> img(w * h * 4);
        aiTexel *src = reinterpret_cast<aiTexel *>(atex->pcData);
        for (int y = 0; y < h; ++y)
        {
            int srcY = flipVertically ? (h - 1 - y) : y;
            for (int x = 0; x < w; ++x)
            {
                aiTexel &t = src[srcY * w + x];
                size_t idx = (y * w + x) * 4;
                img[idx + 0] = t.r;
                img[idx + 1] = t.g;
                img[idx + 2] = t.b;
                img[idx + 3] = t.a;
            }
        }
        glGenTextures(1, &glt);
        glBindTexture(GL_TEXTURE_2D, glt);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        applyTextureParams(generateMipmaps);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, img.data());
        if (generateMipmaps)
            glGenerateMipmap(GL_TEXTURE_2D);
    }
    return glt;
}

unsigned int loadMaterialTexture(aiMaterial *material, const aiScene *scene, const std::string &directory, std::map<std::string, unsigned int> &loaded, bool allowGlobalTextureSearch)
{
    std::string materialName = material->GetName().C_Str();
    std::transform(materialName.begin(), materialName.end(), materialName.begin(), ::tolower);
    const bool isFoliageMaterial = materialName.find("foliage") != std::string::npos;
    std::vector<aiTextureType> textureTypes = {
        aiTextureType_BASE_COLOR,
        aiTextureType_DIFFUSE};

    for (aiTextureType type : textureTypes)
    {
        if (material->GetTextureCount(type) == 0)
            continue;
        aiString str;
        if (material->GetTexture(type, 0, &str) != AI_SUCCESS)
            continue;
        std::string texPath = str.C_Str();
        if (texPath.empty())
            continue;

        if (texPath[0] == '*')
        {
            int texIndex = atoi(texPath.c_str() + 1);
            if (texIndex >= 0 && scene->mNumTextures > (unsigned)texIndex)
            {
                // glTF/GLB UVs and embedded images already use a matching
                // orientation.  The old code accidentally used the unrelated
                // texture-search flag as flipVertically, inverting every map
                // image while leaving its UV coordinates unchanged.
                std::string cacheKey = "embedded:" + std::to_string(texIndex) + (isFoliageMaterial ? ":foliage" : ":mip");
                auto cached = loaded.find(cacheKey);
                if (cached != loaded.end())
                    return cached->second;

                // Mipmaps blend the atlas' nearly opaque white padding into
                // leaf colors and create a pale aura. Foliage stays on the
                // base level; anisotropic filtering remains for other assets.
                unsigned int texture = loadEmbeddedTexture(scene->mTextures[texIndex], allowGlobalTextureSearch, !isFoliageMaterial);
                loaded[cacheKey] = texture;
                return texture;
            }
        }
        else
        {
            unsigned int texture = loadTextureFromFile(texPath, directory, loaded, allowGlobalTextureSearch);
            if (texture != 0)
                return texture;
        }
    }

    return 0;
}

std::vector<unsigned int> loadMaterialTextureFallbacks(const std::string &path, std::unordered_map<std::string, unsigned int> &byName)
{
    std::vector<unsigned int> textures;
    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(path, aiProcess_Triangulate);
    if (!scene)
    {
        std::cout << "Fallback texture source failed: " << path << " " << importer.GetErrorString() << std::endl;
        return textures;
    }

    std::string directory = ".";
    size_t pos = path.find_last_of("/\\");
    if (pos != std::string::npos)
        directory = path.substr(0, pos);

    std::map<std::string, unsigned int> loaded;
    for (unsigned int i = 0; i < scene->mNumMaterials; ++i)
    {
        unsigned int tex = loadMaterialTexture(scene->mMaterials[i], scene, directory, loaded, false);
        if (tex == 0)
            tex = getWhiteTexture();
        textures.push_back(tex);
        std::string name = scene->mMaterials[i]->GetName().C_Str();
        std::transform(name.begin(), name.end(), name.begin(), ::tolower);
        if (!name.empty())
            byName[name] = tex;
        std::cout << "Fallback material[" << i << "] name='" << scene->mMaterials[i]->GetName().C_Str() << "' tex=" << tex << std::endl;
    }
    return textures;
}

void applyTextureParams(bool generateMipmaps)
{
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, generateMipmaps ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    if (generateMipmaps)
    {
        const char *extensions = reinterpret_cast<const char *>(glGetString(GL_EXTENSIONS));
        if (extensions && std::strstr(extensions, "GL_EXT_texture_filter_anisotropic"))
        {
            GLfloat maxAniso = 0.0f;
            glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &maxAniso);
            glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, glm::min(maxAniso, 8.0f));
        }
    }
}
