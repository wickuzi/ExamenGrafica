#include <glad/glad.h>
#include <GLFW/glfw3.h>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>

#include <shader_m.h>
#include <camera.h>
#include <skybox.h>
#include "../assimp/include/assimp/Importer.hpp"
#include "../assimp/include/assimp/scene.h"
#include "../assimp/include/assimp/postprocess.h"
#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <algorithm>
#include <cstring>
#include <filesystem>
#include <array>
#include <unordered_map>
#include <windows.h>
#include <mmsystem.h>
#include <gdiplus.h>
#include <cfloat>

// stb_image for TGA and other formats GDI+ may not support
#define STBI_NO_PKM
#define STB_IMAGE_IMPLEMENTATION
#include "../SOIL2/stb_image.h"

#ifndef GL_TEXTURE_MAX_ANISOTROPY_EXT
#define GL_TEXTURE_MAX_ANISOTROPY_EXT 0x84FE
#endif
#ifndef GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT
#define GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT 0x84FF
#endif

const int MAX_BONES = 100;
const int MAX_BONE_INFLUENCE = 4;

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow *window);
void updateThirdPersonCamera();
void initAudio();
void updateFootstepAudio();
void shutdownAudio();
bool audioCommand(const std::string& command);
unsigned int loadTextureFromJpeg(const wchar_t* path);
struct HudTexture;
HudTexture createTextTexture(const wchar_t* text, const wchar_t* fontName, float fontSize, int width, int height, const Gdiplus::Color& color);
unsigned int loadTextureFromFile(const std::string &path, const std::string &directory, std::map<std::string,unsigned int> &loaded);
unsigned int getWhiteTexture();
unsigned int loadMaterialTexture(aiMaterial* material, const aiScene* scene, const std::string& directory, std::map<std::string,unsigned int>& loaded);
std::vector<unsigned int> loadMaterialTextureFallbacks(const std::string& path, std::unordered_map<std::string, unsigned int>& byName);
struct BoneInfo {
    int id=0;
    glm::mat4 offset=glm::mat4(1.0f);
};
struct MeshData {
    unsigned int VAO=0, VBO=0, EBO=0;
    unsigned int indexCount=0;
    unsigned int texture=0;
    bool hasTexture=false;
    bool hasBones=false;
    glm::vec3 materialColor=glm::vec3(1.0f);
    float materialAlpha=1.0f;
    std::vector<glm::vec3> positions;
    std::vector<unsigned int> indices;
};
struct BoneVertexData {
    std::array<int, MAX_BONE_INFLUENCE> ids;
    std::array<float, MAX_BONE_INFLUENCE> weights;
};
struct KeyPosition {
    glm::vec3 position;
    float timeStamp=0.0f;
};
struct KeyRotation {
    glm::quat orientation;
    float timeStamp=0.0f;
};
struct KeyScale {
    glm::vec3 scale;
    float timeStamp=0.0f;
};
struct AnimChannel {
    std::string name;
    std::vector<KeyPosition> positions;
    std::vector<KeyRotation> rotations;
    std::vector<KeyScale> scales;
};
struct AnimNode {
    std::string name;
    glm::mat4 transform=glm::mat4(1.0f);
    std::vector<AnimNode> children;
};
struct AnimationClip {
    std::string name;
    float duration=0.0f;
    float ticksPerSecond=25.0f;
    AnimNode root;
    std::unordered_map<std::string, AnimChannel> channels;
    bool valid=false;
};
struct AnimationState {
    const AnimationClip* current=nullptr;
    float currentTime=0.0f;
    std::vector<glm::mat4> finalMatrices;
    bool currentLooping=true;
};
struct WalkTriangle {
    glm::vec3 a, b, c;
    float minX, maxX, minZ, maxZ;
    float area;
};
struct HudTexture {
    unsigned int texture=0;
    int width=0;
    int height=0;
};
glm::mat4 aiToGlm(const aiMatrix4x4& from);
void processNode(const aiScene* scene, aiNode* node, const glm::mat4& parentTransform, const std::string &directory, std::vector<MeshData> &meshes, glm::vec3 &aabbMin, glm::vec3 &aabbMax, std::map<std::string,unsigned int> &loaded, std::unordered_map<std::string, BoneInfo>* boneInfoMap, int* boneCounter);
MeshData processMesh(aiMesh* mesh, const aiScene* scene, const glm::mat4& transform, const std::string &directory, glm::vec3 &aabbMin, glm::vec3 &aabbMax, std::map<std::string,unsigned int> &loaded, std::unordered_map<std::string, BoneInfo>* boneInfoMap, int* boneCounter);
std::vector<MeshData> loadModel(const std::string &path, glm::vec3 &outAABBMin, glm::vec3 &outAABBMax, std::unordered_map<std::string, BoneInfo>* boneInfoMap=nullptr, int* boneCounter=nullptr, glm::mat4* outGlobalInverse=nullptr);
AnimationClip loadAnimationClip(const std::string& path, const std::string& name);
void updateAnimation(AnimationState& state, const AnimationClip* clip, float deltaSeconds, const std::unordered_map<std::string, BoneInfo>& boneInfoMap, int boneCount, const glm::mat4& globalInverseTransform, bool looping);
void calculateBoneTransforms(const AnimNode& node, const glm::mat4& parentTransform, const AnimationClip& clip, std::vector<glm::mat4>& finalMatrices, const std::unordered_map<std::string, BoneInfo>& boneInfoMap);
glm::mat4 interpolateChannelTransform(const AnimChannel& channel, float animationTime);
const AnimationClip* findClip(const std::unordered_map<std::string, AnimationClip>& clips, const std::string& name);
std::vector<WalkTriangle> buildWalkTriangles(const std::vector<MeshData>& meshes, const glm::mat4& modelTransform);
glm::vec3 findSpawnPoint();
bool findGroundHeightAt(float x, float z, float maxStepUp, float& outY);
float findGroundHeight(float x, float z, float fallbackY);
glm::vec3 closestPointOnTriangle(const glm::vec3& p, const glm::vec3& a, const glm::vec3& b, const glm::vec3& c);
bool findWallAttachment(const std::vector<MeshData>& meshes, const glm::mat4& modelTransform, const glm::vec3& anchor, const glm::vec3& preferredNormal, glm::vec3& outPosition, glm::vec3& outNormal);
glm::mat4 makeWallModel(const glm::vec3& position, const glm::vec3& normal, const glm::vec3& scale);
void applyTextureParams(bool generateMipmaps);
// create a simple cube mesh for light source
void createCube(unsigned int &VAO, unsigned int &indexCount);
void createQuad(unsigned int &VAO, unsigned int &indexCount);
void createDisk(unsigned int &VAO, unsigned int &indexCount);

// settings
const unsigned int SCR_WIDTH = 1280;
const unsigned int SCR_HEIGHT = 720;

// camera
Camera camera(glm::vec3(0.0f, 2.2f, 5.5f));
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;
float thirdPersonYaw = -90.0f;
float thirdPersonPitch = -2.0f;
float thirdPersonDistance = 2.30f;
float thirdPersonHeight = 1.20f;
float thirdPersonShoulderOffset = 0.42f;
float thirdPersonLookOffset = 0.12f;
glm::vec3 playerPosition(0.0f, 0.0f, 0.0f);
float playerGroundY = 0.0f;
float playerYaw = 180.0f;
std::vector<WalkTriangle> walkTriangles;
bool jumpRequested = false;
bool spaceWasPressed = false;
bool eWasPressed = false;
bool saveMenuOpen = false;
bool turnAnimationActive = false;
bool playerIsMoving = false;
bool footstepsPlaying = false;
float footstepTimer = 0.0f;

// timing
float deltaTime = 0.0f;	
float lastFrame = 0.0f;

const glm::vec3 SAVE_POINT_POSITION = glm::vec3(4.5f, 1.75f, 1.5f);
const glm::vec3 SAVE_POINT_WALL_NORMAL = glm::vec3(1.0f, 0.0f, 0.0f);
glm::vec3 savePointPosition = SAVE_POINT_POSITION;
glm::vec3 savePointNormal = SAVE_POINT_WALL_NORMAL;
const float SAVE_POINT_INTERACT_RADIUS = 1.45f;
const glm::vec3 FOG_COLOR(0.72f, 0.73f, 0.66f);
const float JAMES_FBX_SKIN_SCALE = 0.01f;
const glm::vec3 JAMES_FBX_SKIN_OFFSET(0.0f, -0.90f, 0.0f);
std::vector<unsigned int> jamesFallbackTextures;
std::unordered_map<std::string, unsigned int> jamesFallbackTexturesByName;

// default orientation tweak (degrees) to make model face +Z upright; adjust if needed
const float MODEL_ROT_X = 0.0f;
const float MODEL_ROT_Y = 0.0f;
const float MODEL_ROT_Z = 0.0f; // try X=90, Y=180, Z=0 for upright facing +Z
const float MAP_TARGET_SIZE = 38.0f;

unsigned int loadTextureFromJpeg(const wchar_t* path)
{
    Gdiplus::Bitmap bitmap(path);
    if (bitmap.GetLastStatus() != Gdiplus::Ok)
    {
        std::wstring w(path);
        std::string s(w.begin(), w.end());
        std::cout << "Failed to load texture: " << s << std::endl;
        return 0;
    }

    //bitmap.RotateFlip(Gdiplus::RotateNoneFlipY);

    const UINT width = bitmap.GetWidth();
    const UINT height = bitmap.GetHeight();
    Gdiplus::Rect rect(0, 0, width, height);
    Gdiplus::BitmapData bitmapData{};
    if (bitmap.LockBits(&rect, Gdiplus::ImageLockModeRead, PixelFormat32bppARGB, &bitmapData) != Gdiplus::Ok)
    {
        std::cout << "Failed to read texture pixels" << std::endl;
        return 0;
    }

    std::vector<unsigned char> pixels(width * height * 4);
    auto* srcBase = static_cast<unsigned char*>(bitmapData.Scan0);
    for (UINT y = 0; y < height; ++y)
    {
        const unsigned char* srcRow = srcBase + y * bitmapData.Stride;
        unsigned char* dstRow = pixels.data() + y * width * 4;
        for (UINT x = 0; x < width; ++x)
        {
            const unsigned char* src = srcRow + x * 4;
            unsigned char* dst = dstRow + x * 4;
            dst[0] = src[2];
            dst[1] = src[1];
            dst[2] = src[0];
            dst[3] = src[3];
        }
    }
    bitmap.UnlockBits(&bitmapData);

    unsigned int texture = 0;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    applyTextureParams(true);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, static_cast<int>(width), static_cast<int>(height), 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
    glGenerateMipmap(GL_TEXTURE_2D);

    return texture;
}

HudTexture createTextTexture(const wchar_t* text, const wchar_t* fontName, float fontSize, int width, int height, const Gdiplus::Color& color)
{
    HudTexture result{};
    Gdiplus::Bitmap bitmap(width, height, PixelFormat32bppARGB);
    Gdiplus::Graphics graphics(&bitmap);
    graphics.Clear(Gdiplus::Color(0, 0, 0, 0));
    graphics.SetTextRenderingHint(Gdiplus::TextRenderingHintAntiAliasGridFit);

    Gdiplus::FontFamily requestedFamily(fontName);
    const Gdiplus::FontFamily* family = requestedFamily.IsAvailable()
        ? &requestedFamily
        : Gdiplus::FontFamily::GenericSerif();
    Gdiplus::Font font(family, fontSize, Gdiplus::FontStyleRegular, Gdiplus::UnitPixel);
    Gdiplus::SolidBrush brush(color);
    Gdiplus::StringFormat format;
    format.SetAlignment(Gdiplus::StringAlignmentNear);
    format.SetLineAlignment(Gdiplus::StringAlignmentNear);
    Gdiplus::RectF layout(0.0f, 0.0f, static_cast<Gdiplus::REAL>(width), static_cast<Gdiplus::REAL>(height));
    graphics.DrawString(text, -1, &font, layout, &format, &brush);

    Gdiplus::Rect rect(0, 0, width, height);
    Gdiplus::BitmapData bitmapData{};
    if (bitmap.LockBits(&rect, Gdiplus::ImageLockModeRead, PixelFormat32bppARGB, &bitmapData) != Gdiplus::Ok)
        return result;

    std::vector<unsigned char> pixels(width * height * 4);
    auto* srcBase = static_cast<unsigned char*>(bitmapData.Scan0);
    for (int y = 0; y < height; ++y) {
        const unsigned char* srcRow = srcBase + y * bitmapData.Stride;
        unsigned char* dstRow = pixels.data() + y * width * 4;
        for (int x = 0; x < width; ++x) {
            const unsigned char* src = srcRow + x * 4;
            unsigned char* dst = dstRow + x * 4;
            dst[0] = src[2];
            dst[1] = src[1];
            dst[2] = src[0];
            dst[3] = src[3];
        }
    }
    bitmap.UnlockBits(&bitmapData);

    glGenTextures(1, &result.texture);
    glBindTexture(GL_TEXTURE_2D, result.texture);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
    result.width = width;
    result.height = height;
    return result;
}

enum GameState
{
    MENU,
    PLAYING,
    PAUSED
};

GameState currentState = MENU;

int main()
{


    // Mostrar directorio de trabajo actual
    char cwd[MAX_PATH];
    GetCurrentDirectoryA(MAX_PATH, cwd);
    std::cout << "Current working directory: " << cwd << std::endl;
    
    // Listar archivos en el directorio actual
    std::cout << "Files in current directory:" << std::endl;
    for (const auto& entry : std::filesystem::directory_iterator(".")) {
        std::cout << "  - " << entry.path().filename().string() << std::endl;
    }
    // glfw: initialize and configure
    // ------------------------------
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_SAMPLES, 8);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // glfw window creation
    // --------------------
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Silent Hill 2", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);

    // tell GLFW to capture our mouse
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    updateThirdPersonCamera();

    // glad: load all OpenGL function pointers
    // ---------------------------------------
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    // configure global opengl state
    // -----------------------------
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_MULTISAMPLE);

    // determine resource directory (where shaders/textures live)
    char exePathArr[MAX_PATH];
    GetModuleFileNameA(NULL, exePathArr, MAX_PATH);
    std::filesystem::path exePath(exePathArr);
    std::filesystem::path resourceDir = exePath.parent_path() / "Resource Files";
    resourceDir = std::filesystem::weakly_canonical(resourceDir);
    // build and compile our shader program using resourceDir
    // ------------------------------------
    Shader lightingShader((resourceDir / "2.2.basic_lighting.vs").string().c_str(), (resourceDir / "2.2.basic_lighting.fs").string().c_str());
    Shader lightCubeShader((resourceDir / "2.2.light_cube.vs").string().c_str(), (resourceDir / "2.2.light_cube.fs").string().c_str());
    Shader skyboxShader((resourceDir / "skybox.vs").string().c_str(), (resourceDir / "skybox.fs").string().c_str());

    Gdiplus::GdiplusStartupInput gdiplusStartupInput;
    ULONG_PTR gdiplusToken = 0;
    if (Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL) != Gdiplus::Ok)
    {
        std::cout << "Failed to initialize GDI+" << std::endl;
        glfwTerminate();
        return -1;
    }

    // legacy textures removed — model uses its own textures from `assets/`.
    // optional legacy textures may be missing; ignore if not present

    // load skybox with cubemap textures
 std::vector<std::string> skyboxFaces = {
    "skybox/Sky_AllSky_Overcast4_Low_Cam_2_Left+X.png",      // right (+X)
    "skybox/Sky_AllSky_Overcast4_Low_Cam_3_Right-X.png",     // left (-X)

    "skybox/Sky_AllSky_Overcast4_Low_Cam_4_Up+Y.png",        // top (+Y)
    "skybox/Sky_AllSky_Overcast4_Low_Cam_5_Down-Y.png",      // bottom (-Y)

    "skybox/Sky_AllSky_Overcast4_Low_Cam_0_Front+Z.png",     // front
    "skybox/Sky_AllSky_Overcast4_Low_Cam_1_Back-Z.png"       // back
};
    Skybox skybox(skyboxFaces);

    lightingShader.use();
    lightingShader.setInt("texture1", 0);

    // set basic material properties for Phong shading
    lightingShader.setFloat("material_shininess", 32.0f);
    lightingShader.setFloat("material_specularStrength", 0.12f);
    lightingShader.setFloat("material_ambientStrength", 0.48f);
    lightingShader.setVec3("fogColor", FOG_COLOR);
    lightingShader.setFloat("fogDensity", 0.108f);
    lightingShader.setFloat("fogStart", 2.6f);
    lightingShader.setFloat("fogEnd", 23.0f);
    lightingShader.setInt("fogEnabled", 1);
    lightingShader.setFloat("objectAlpha", 1.0f);
    lightingShader.setFloat("alphaCutoff", 0.38f);
    lightingShader.setFloat("emissiveStrength", 0.0f);

    unsigned int savePaperVAO = 0;
    unsigned int savePaperIndexCount = 0;
    createQuad(savePaperVAO, savePaperIndexCount);
    unsigned int saveDiskVAO = 0;
    unsigned int saveDiskIndexCount = 0;
    createDisk(saveDiskVAO, saveDiskIndexCount);
    unsigned int hudQuadVAO = 0;
    unsigned int hudQuadIndexCount = 0;
    createQuad(hudQuadVAO, hudQuadIndexCount);

    //richard's custom HUD textures
    HudTexture titleText;
    HudTexture startText;
    HudTexture exitText;
    HudTexture menuWallpaper;
    
    std::filesystem::path wallpaperPath = resourceDir.parent_path() / "Resource Files" / "shwallpaper.jpeg";
    if (!std::filesystem::exists(wallpaperPath)) {
        wallpaperPath = std::filesystem::path("Resource Files") / "shwallpaper.jpeg";
    }
    unsigned int wallTexID = loadTextureFromJpeg(wallpaperPath.wstring().c_str());

    // Si la carga fue exitosa, llenamos los datos de la estructura
    if (wallTexID != 0) {
        menuWallpaper.texture = wallTexID;
        menuWallpaper.width = SCR_WIDTH;   // Se estira al ancho de tu pantalla
        menuWallpaper.height = SCR_HEIGHT; // Se estira al alto de tu pantalla
        std::cout << "Wallpaper del menu cargado exitosamente con GDI+." << std::endl;
    } else {
        std::cout << "Error: No se pudo cargar el wallpaper del menu." << std::endl;
    }

    HudTexture saveTitleText = createTextTexture(L"SAVE GAME", L"Georgia", 42.0f, 400, 72, Gdiplus::Color(245, 245, 238, 228));
    HudTexture saveEmptyText = createTextTexture(L"EMPTY SLOT", L"Georgia", 24.0f, 320, 48, Gdiplus::Color(205, 222, 216, 210));
    HudTexture saveLocationText = createTextTexture(L"SILENT HILL ROAD", L"Georgia", 23.0f, 430, 64, Gdiplus::Color(220, 218, 210, 212));
    HudTexture saveTimeText = createTextTexture(L"19:42    2024/09/06    10:29", L"Georgia", 24.0f, 490, 48, Gdiplus::Color(210, 208, 200, 180));
    HudTexture savePromptText = createTextTexture(L"PRESS E TO RETURN", L"Georgia", 19.0f, 300, 40, Gdiplus::Color(230, 218, 210, 190));
    HudTexture saveInteractText = createTextTexture(L"PRESS E", L"Georgia", 20.0f, 112, 34, Gdiplus::Color(245, 230, 220, 218));
    initAudio();

    // create title and menu textures by ropchard
        titleText =
    createTextTexture(
        L"SILENT HILL 2",
        L"Georgia",
        72,
        800,
        100,
        Gdiplus::Color(255,255,255,255)
    );

    startText =
    createTextTexture(
        L"PRESS ENTER TO START",
        L"Georgia",
        36,
        500,
        50,
        Gdiplus::Color(255,220,220,220)
    );

    exitText =
    createTextTexture(
        L"ESC TO EXIT",
        L"Georgia",
        30,
        300,
        50,
        Gdiplus::Color(255,180,180,180)
    );


    auto drawHudQuad = [&](unsigned int texture, float x, float y, float width, float height, const glm::vec3& color, float alpha) {
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(x + width * 0.5f, y + height * 0.5f, 0.0f));
        model = glm::scale(model, glm::vec3(width, height, 1.0f));
        lightingShader.setMat4("model", model);
        lightingShader.setVec3("objectColor", color);
        lightingShader.setFloat("objectAlpha", alpha);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture);
        glBindVertexArray(hudQuadVAO);
        glDrawElements(GL_TRIANGLES, (GLsizei)hudQuadIndexCount, GL_UNSIGNED_INT, 0);
    };

    // Load James model
    std::string jamesModelPath;
    std::vector<std::filesystem::path> jamesCandidates = {
        std::filesystem::path("models") / "jamesanimations" / "jamessunderland.fbx",
        std::filesystem::path("..") / "models" / "jamesanimations" / "jamessunderland.fbx",
        resourceDir.parent_path() / "models" / "jamesanimations" / "jamessunderland.fbx",
        resourceDir.parent_path().parent_path() / "models" / "jamesanimations" / "jamessunderland.fbx",
        std::filesystem::path("models") / "james" / "james_sunderland.glb",
        std::filesystem::path("..") / "models" / "james" / "james_sunderland.glb",
        resourceDir.parent_path() / "models" / "james" / "james_sunderland.glb",
        resourceDir.parent_path().parent_path() / "models" / "james" / "james_sunderland.glb"
    };
    for (auto &p : jamesCandidates) {
        if (std::filesystem::exists(p)) {
            jamesModelPath = p.string();
            std::cout << "Found James model at: " << jamesModelPath << std::endl;
            break;
        }
    }

    std::vector<std::filesystem::path> jamesTextureFallbackCandidates = {
        std::filesystem::path("models") / "james" / "james_sunderland.glb",
        std::filesystem::path("..") / "models" / "james" / "james_sunderland.glb",
        resourceDir.parent_path() / "models" / "james" / "james_sunderland.glb",
        resourceDir.parent_path().parent_path() / "models" / "james" / "james_sunderland.glb"
    };
    for (auto &p : jamesTextureFallbackCandidates) {
        if (std::filesystem::exists(p)) {
            jamesFallbackTextures = loadMaterialTextureFallbacks(p.string(), jamesFallbackTexturesByName);
            std::cout << "Loaded James fallback textures from " << p.string() << ": " << jamesFallbackTextures.size() << std::endl;
            break;
        }
    }

    std::cout << "=== Model Loading ===" << std::endl;
    std::cout << "James model path: " << (jamesModelPath.empty() ? "NOT FOUND" : jamesModelPath) << std::endl;

    // Load James model if found
    glm::vec3 jamesAABBMin(FLT_MAX);
    glm::vec3 jamesAABBMax(-FLT_MAX);
    std::vector<MeshData> jamesMeshes;
    std::unordered_map<std::string, BoneInfo> jamesBoneInfo;
    int jamesBoneCount = 0;
    glm::mat4 jamesGlobalInverseTransform(1.0f);
    float jamesRenderScale = 0.8f;
    if (!jamesModelPath.empty()) {
        jamesMeshes = loadModel(jamesModelPath, jamesAABBMin, jamesAABBMax, &jamesBoneInfo, &jamesBoneCount, &jamesGlobalInverseTransform);
        glm::vec3 jamesSize = jamesAABBMax - jamesAABBMin;
        float jamesHeight = glm::max(jamesSize.y, 0.001f);
        jamesRenderScale = 1.78f / jamesHeight;
        std::cout << "Loaded James meshes: " << jamesMeshes.size() << " bones=" << jamesBoneCount << std::endl;
        std::cout << "James size=(" << jamesSize.x << ", " << jamesSize.y << ", " << jamesSize.z << ") renderScale=" << jamesRenderScale << std::endl;
        for (size_t i = 0; i < jamesMeshes.size(); ++i) {
            std::cout << " James Mesh[" << i << "] indices=" << jamesMeshes[i].indexCount << " tex=" << jamesMeshes[i].texture << std::endl;
        }
    }

    std::unordered_map<std::string, AnimationClip> jamesAnimations;
    std::filesystem::path animDir = resourceDir.parent_path() / "models" / "jamesanimations";
    if (!std::filesystem::exists(animDir)) {
        animDir = resourceDir.parent_path().parent_path() / "models" / "jamesanimations";
    }
    std::vector<std::pair<std::string, std::string>> animFiles = {
        {"idle", "idle.fbx"},
        {"walking", "walking.fbx"},
        {"strafe_left", "left strafe walking.fbx"},
        {"strafe_right", "right strafe walking.fbx"},
        {"turn_left", "left turn 90.fbx"},
        {"turn_right", "right turn 90.fbx"},
        {"jump", "jump.fbx"}
    };
    for (const auto& entry : animFiles) {
        std::filesystem::path p = animDir / entry.second;
        if (std::filesystem::exists(p)) {
            AnimationClip clip = loadAnimationClip(p.string(), entry.first);
            if (clip.valid) {
                jamesAnimations[entry.first] = clip;
                std::cout << "Loaded animation: " << entry.first << " from " << p.string() << std::endl;
            }
        } else {
            std::cout << "Animation not found: " << p.string() << std::endl;
        }
    }
    AnimationState jamesAnimState;
    jamesAnimState.finalMatrices.assign(MAX_BONES, glm::mat4(1.0f));
    jamesAnimState.current = findClip(jamesAnimations, "idle");

    // Load map/scene if present. Supports either a clean models/map layout or the current SketchUp export under models/james.
    std::string mapModelPath;
    std::vector<std::filesystem::path> mapCandidates = {
        std::filesystem::path("models") / "map" / "model.dae",
        std::filesystem::path("models") / "map" / "scene.dae",
        std::filesystem::path("models") / "model.dae",
        std::filesystem::path("models") / "james" / "model.dae",
        resourceDir.parent_path() / "models" / "map" / "model.dae",
        resourceDir.parent_path() / "models" / "map" / "scene.dae",
        resourceDir.parent_path() / "models" / "model.dae",
        resourceDir.parent_path() / "models" / "james" / "model.dae"
    };
    for (auto &p : mapCandidates) {
        if (std::filesystem::exists(p)) {
            mapModelPath = p.string();
            std::cout << "Found map model at: " << mapModelPath << std::endl;
            break;
        }
    }

    glm::vec3 mapAABBMin(FLT_MAX);
    glm::vec3 mapAABBMax(-FLT_MAX);
    std::vector<MeshData> mapMeshes;
    float mapScale = 1.0f;
    bool mapUsesZUp = false;
    glm::mat4 mapModelTransform = glm::mat4(1.0f);
    if (!mapModelPath.empty()) {
        mapMeshes = loadModel(mapModelPath, mapAABBMin, mapAABBMax);
        glm::vec3 mapSize = mapAABBMax - mapAABBMin;
        std::string mapExt = std::filesystem::path(mapModelPath).extension().string();
        std::transform(mapExt.begin(), mapExt.end(), mapExt.begin(), ::tolower);
        mapUsesZUp = false;
        float mapHorizontalSize = glm::max(mapSize.x, mapSize.z);
        if (mapHorizontalSize > 0.001f) {
            mapScale = MAP_TARGET_SIZE / mapHorizontalSize;
        }
        playerGroundY = 0.0f;
        playerPosition.y = playerGroundY;
        std::cout << "Loaded map meshes: " << mapMeshes.size()
                  << " size=(" << mapSize.x << ", " << mapSize.y << ", " << mapSize.z << ")"
                  << " scale=" << mapScale
                  << " externalAxisFix=false" << std::endl;
    } else {
        std::cout << "Map model path: NOT FOUND" << std::endl;
    }

    if (!mapMeshes.empty()) {
        glm::vec3 mapCenter = (mapAABBMin + mapAABBMax) * 0.5f;
        mapModelTransform = glm::scale(mapModelTransform, glm::vec3(mapScale));
        mapModelTransform = glm::translate(mapModelTransform, glm::vec3(-mapCenter.x, -mapAABBMin.y, -mapCenter.z));
        walkTriangles = buildWalkTriangles(mapMeshes, mapModelTransform);
        playerPosition = findSpawnPoint();
        playerGroundY = findGroundHeight(playerPosition.x, playerPosition.z, playerPosition.y);
        playerPosition.y = playerGroundY;
        std::cout << "Walkable triangles: " << walkTriangles.size()
                  << " spawn=(" << playerPosition.x << ", " << playerPosition.y << ", " << playerPosition.z << ")"
                  << " initialGroundY=" << playerGroundY << std::endl;
    }

    std::cout << "==================" << std::endl;


// render loop
    // -----------
    while (!glfwWindowShouldClose(window))
    {
        // per-frame time logic
        // --------------------
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // input
        // -----
        processInput(window);

    // =================================================================
    // CASO A: REGLAS PARA EL MENÚ PRINCIPAL
    // =================================================================
            if (currentState == MENU)
            {
                // Limpiar buffers
                glClearColor(0.05f, 0.05f, 0.05f, 1.0f);
                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

                glDisable(GL_DEPTH_TEST);
                glDisable(GL_CULL_FACE);

                // Forzar el cursor visible en menús
                glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);

                // Configurar proyección Ortogonal para HUD/Texturas 2D
                glm::mat4 orthoProjection = glm::ortho(0.0f, (float)SCR_WIDTH, (float)SCR_HEIGHT, 0.0f, -1.0f, 1.0f);
                glm::mat4 viewIdentity = glm::mat4(1.0f);

                lightingShader.use();
                lightingShader.setMat4("projection", orthoProjection);
                lightingShader.setMat4("view", viewIdentity);
                lightingShader.setInt("fogEnabled", 0);
                lightingShader.setInt("useSkinning", 0);
                lightingShader.setFloat("objectAlpha", 1.0f);
                lightingShader.setFloat("emissiveStrength", 0.0f);
                lightingShader.setInt("numPointLights", 0);  // Importante: desactivar luces puntuales
                
                // Configurar luz direccional por defecto
                lightingShader.setVec3("dirLight_direction", 0.0f, 0.0f, -1.0f);
                lightingShader.setVec3("dirLight_color", 1.0f, 1.0f, 1.0f);
                lightingShader.setFloat("material_ambientStrength", 1.0f);
                lightingShader.setFloat("material_specularStrength", 0.0f);
                
                // Verificar que el wallpaper se cargó correctamente
                if (menuWallpaper.texture == 0) {
                    std::cout << "ERROR: menuWallpaper texture is 0, trying to reload..." << std::endl;
                    std::filesystem::path wallpaperPath = resourceDir.parent_path() / "Resource Files" / "shwallpaper.jpeg";
                    if (!std::filesystem::exists(wallpaperPath)) {
                        wallpaperPath = std::filesystem::path("Resource Files") / "shwallpaper.jpeg";
                    }
                    unsigned int wallTexID = loadTextureFromJpeg(wallpaperPath.wstring().c_str());
                    if (wallTexID != 0) {
                        menuWallpaper.texture = wallTexID;
                        menuWallpaper.width = SCR_WIDTH;
                        menuWallpaper.height = SCR_HEIGHT;
                        std::cout << "Wallpaper recargado exitosamente." << std::endl;
                    } else {
                        std::cout << "ERROR: No se pudo cargar el wallpaper." << std::endl;
                    }
                }
                
                // Dibujar wallpaper (fondo)
                if (menuWallpaper.texture != 0) {
                    drawHudQuad(menuWallpaper.texture, 0.0f, 0.0f,
                                (float)SCR_WIDTH, (float)SCR_HEIGHT,
                                glm::vec3(1.0f, 1.0f, 1.0f), 1.0f);
                } else {
                    // Fallback: color sólido oscuro
                    drawHudQuad(getWhiteTexture(), 0.0f, 0.0f,
                                (float)SCR_WIDTH, (float)SCR_HEIGHT,
                                glm::vec3(0.1f, 0.05f, 0.05f), 1.0f);
                }

                // Configurar para textos (blending adecuado)
                glEnable(GL_BLEND);
                glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                
                // Dibujar elementos centrados en pantalla
                if (titleText.texture != 0) {
                    float titleX = (SCR_WIDTH - titleText.width) / 2.0f;
                    float titleY = 150.0f; 
                    drawHudQuad(titleText.texture, titleX, titleY, 
                            (float)titleText.width, (float)titleText.height, 
                            glm::vec3(1.2f, 1.2f, 1.2f), 1.0f);
                }

                if (startText.texture != 0) {
                    float startX = (SCR_WIDTH - startText.width) / 2.0f;
                    float startY = 400.0f;
                    drawHudQuad(startText.texture, startX, startY, 
                            (float)startText.width, (float)startText.height, 
                            glm::vec3(1.2f, 1.2f, 1.2f), 1.0f);
                }

                if (exitText.texture != 0) {
                    float exitX = (SCR_WIDTH - exitText.width) / 2.0f;
                    float exitY = 500.0f;
                    drawHudQuad(exitText.texture, exitX, exitY, 
                            (float)exitText.width, (float)exitText.height, 
                            glm::vec3(1.2f, 1.2f, 1.2f), 1.0f);
                }
        }
        // =================================================================
        // CASO B: REGLAS PARA EL JUEGO ACTIVO (PLAYING)
        // =================================================================
        else if (currentState == PLAYING)
        {
            // Ocultar y capturar el cursor para control de cámara libre
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

            // Actualizaciones lógicas del mundo
            updateFootstepAudio();
            updateThirdPersonCamera();

            // 1. Manejo del árbol de Animación de James
            const AnimationClip* desiredClip = findClip(jamesAnimations, "idle");
            bool desiredLooping = true;
            bool forwardPressed = glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS;
            bool backwardPressed = glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS;
            bool leftPressed = glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS;
            bool rightPressed = glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS;

            if (jumpRequested && findClip(jamesAnimations, "jump")) {
                desiredClip = findClip(jamesAnimations, "jump");
                desiredLooping = false;
            } else if ((forwardPressed || backwardPressed) && leftPressed && !rightPressed && findClip(jamesAnimations, "strafe_left")) {
                desiredClip = findClip(jamesAnimations, "strafe_left");
            } else if ((forwardPressed || backwardPressed) && rightPressed && !leftPressed && findClip(jamesAnimations, "strafe_right")) {
                desiredClip = findClip(jamesAnimations, "strafe_right");
            } else if ((forwardPressed || backwardPressed) && findClip(jamesAnimations, "walking")) {
                desiredClip = findClip(jamesAnimations, "walking");
            } else if ((leftPressed || turnAnimationActive) && !rightPressed && findClip(jamesAnimations, "turn_left")) {
                desiredClip = findClip(jamesAnimations, "turn_left");
                desiredLooping = false;
            } else if ((rightPressed || turnAnimationActive) && !leftPressed && findClip(jamesAnimations, "turn_right")) {
                desiredClip = findClip(jamesAnimations, "turn_right");
                desiredLooping = false;
            }

            updateAnimation(jamesAnimState, desiredClip, deltaTime, jamesBoneInfo, jamesBoneCount, jamesGlobalInverseTransform, desiredLooping);
            turnAnimationActive = desiredClip && !desiredLooping && desiredClip != findClip(jamesAnimations, "jump") && jamesAnimState.currentTime < desiredClip->duration - 1.0f;
            
            for (auto& matrix : jamesAnimState.finalMatrices) {
                matrix = glm::translate(glm::mat4(1.0f), JAMES_FBX_SKIN_OFFSET) *
                         glm::scale(glm::mat4(1.0f), glm::vec3(JAMES_FBX_SKIN_SCALE)) *
                         matrix;
            }
            jumpRequested = false;

            // Limpiar buffers para el frame 3D con el color de la niebla
            glClearColor(FOG_COLOR.r, FOG_COLOR.g, FOG_COLOR.b, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            // Transformaciones espaciales básicas 3D
            glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
            glm::mat4 view = camera.GetViewMatrix();

            glEnable(GL_DEPTH_TEST);

            // Renderizar Skybox
            skybox.render(skyboxShader.ID, view, projection);

            // Configuración del Shader de Iluminación para elementos 3D estáticos
            lightingShader.use();
            lightingShader.setMat4("projection", projection);
            lightingShader.setMat4("view", view);
            lightingShader.setInt("fogEnabled", 1);
            lightingShader.setFloat("objectAlpha", 1.0f);
            lightingShader.setFloat("emissiveStrength", 0.0f);
            lightingShader.setVec3("objectColor", 1.0f, 1.0f, 1.0f);
            lightingShader.setVec3("viewPos", camera.Position);
            lightingShader.setInt("useSkinning", 0);

            // Luz Direccional global del entorno
            lightingShader.setVec3("dirLight_direction", -0.2f, -1.0f, -0.3f);
            lightingShader.setVec3("dirLight_color", 0.88f, 0.88f, 0.78f);

            // Configuración de la luz puntal roja del punto de guardado
            int numPoints = 1;
            lightingShader.setInt("numPointLights", numPoints);
            {
                glm::vec3 saveWorld = savePointPosition + savePointNormal * 0.35f;
                glm::vec3 saveGlow = glm::vec3(1.0f, 0.08f, 0.04f) * 0.75f;
                std::string base = "pointLights[0].";
                lightingShader.setVec3(base + "position", saveWorld);
                lightingShader.setVec3(base + "color", saveGlow);
                lightingShader.setFloat(base + "constant", 1.0f);
                lightingShader.setFloat(base + "linear", 0.08f);
                lightingShader.setFloat(base + "quadratic", 0.035f);
            }

            // 2. Dibujar el mapa / escenario
            if (!mapMeshes.empty()) {
                glEnable(GL_CULL_FACE);
                glCullFace(GL_BACK);
                glFrontFace(GL_CCW);
                lightingShader.setMat4("model", mapModelTransform);
                lightingShader.setFloat("alphaCutoff", 0.70f);
                for (auto &m : mapMeshes) {
                    if (!m.hasTexture) continue;
                    lightingShader.setVec3("objectColor", m.materialColor);
                    lightingShader.setFloat("objectAlpha", m.materialAlpha);
                    glActiveTexture(GL_TEXTURE0);
                    glBindTexture(GL_TEXTURE_2D, m.texture);
                    glBindVertexArray(m.VAO);
                    glDrawElements(GL_TRIANGLES, (GLsizei)m.indexCount, GL_UNSIGNED_INT, 0);
                }
                glDisable(GL_CULL_FACE);
                lightingShader.setVec3("objectColor", 1.0f, 1.0f, 1.0f);
                lightingShader.setFloat("objectAlpha", 1.0f);
                lightingShader.setFloat("alphaCutoff", 0.38f);
                lightingShader.setInt("useSkinning", 0);
            }

            // 3. Dibujar el modelo animado de James (Skinning habilitado)
            if (!jamesMeshes.empty()) {
                glm::mat4 jamesModel = glm::mat4(1.0f);
                glm::vec3 jamesCenter = (jamesAABBMin + jamesAABBMax) * 0.5f;
                float jamesMinY = jamesAABBMin.y;
                jamesModel = glm::translate(jamesModel, playerPosition);
                jamesModel = glm::rotate(jamesModel, glm::radians(MODEL_ROT_X), glm::vec3(1.0f, 0.0f, 0.0f));
                jamesModel = glm::rotate(jamesModel, glm::radians(MODEL_ROT_Y + playerYaw), glm::vec3(0.0f, 1.0f, 0.0f));
                jamesModel = glm::rotate(jamesModel, glm::radians(MODEL_ROT_Z), glm::vec3(0.0f, 0.0f, 1.0f));
                jamesModel = glm::scale(jamesModel, glm::vec3(jamesRenderScale));
                jamesModel = glm::translate(jamesModel, glm::vec3(-jamesCenter.x, -jamesMinY, -jamesCenter.z));
                
                lightingShader.setMat4("model", jamesModel);
                lightingShader.setInt("useSkinning", jamesBoneCount > 0 && jamesAnimState.current ? 1 : 0);
                
                for (int i = 0; i < glm::min(jamesBoneCount, MAX_BONES); ++i) {
                    lightingShader.setMat4("finalBonesMatrices[" + std::to_string(i) + "]", jamesAnimState.finalMatrices[i]);
                }
                for (auto &m : jamesMeshes) {
                    lightingShader.setVec3("objectColor", m.materialColor);
                    lightingShader.setFloat("objectAlpha", m.materialAlpha);
                    glActiveTexture(GL_TEXTURE0);
                    glBindTexture(GL_TEXTURE_2D, m.texture);
                    glBindVertexArray(m.VAO);
                    glDrawElements(GL_TRIANGLES, (GLsizei)m.indexCount, GL_UNSIGNED_INT, 0);
                }
                lightingShader.setVec3("objectColor", 1.0f, 1.0f, 1.0f);
                lightingShader.setFloat("objectAlpha", 1.0f);
                lightingShader.setInt("useSkinning", 0);
            }

            // 4. Dibujar geometrías físicas del punto de guardado (Hojas de papel y discos)
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, getWhiteTexture());
            glBindVertexArray(savePaperVAO);

            glm::mat4 savePaperModel = glm::mat4(1.0f);
            savePaperModel = makeWallModel(savePointPosition, savePointNormal, glm::vec3(0.56f, 0.34f, 1.0f));
            lightingShader.setMat4("model", savePaperModel);
            lightingShader.setVec3("objectColor", 1.0f, 0.04f, 0.025f);
            lightingShader.setFloat("emissiveStrength", 0.45f);
            glDrawElements(GL_TRIANGLES, (GLsizei)savePaperIndexCount, GL_UNSIGNED_INT, 0);

            glm::mat4 saveRimModel = glm::mat4(1.0f);
            saveRimModel = makeWallModel(savePointPosition + glm::vec3(0.0f, 0.07f, 0.0f) + savePointNormal * 0.003f, savePointNormal, glm::vec3(0.115f, 0.115f, 1.0f));
            lightingShader.setMat4("model", saveRimModel);
            lightingShader.setVec3("objectColor", 0.015f, 0.012f, 0.012f);
            lightingShader.setFloat("emissiveStrength", 0.0f);
            glBindVertexArray(saveDiskVAO);
            glDrawElements(GL_TRIANGLES, (GLsizei)saveDiskIndexCount, GL_UNSIGNED_INT, 0);

            glm::mat4 saveButtonModel = glm::mat4(1.0f);
            saveButtonModel = makeWallModel(savePointPosition + glm::vec3(0.0f, 0.07f, 0.0f) + savePointNormal * 0.006f, savePointNormal, glm::vec3(0.078f, 0.078f, 1.0f));
            lightingShader.setMat4("model", saveButtonModel);
            lightingShader.setVec3("objectColor", 1.0f, 0.96f, 0.86f);
            lightingShader.setFloat("emissiveStrength", 0.65f);
            glDrawElements(GL_TRIANGLES, (GLsizei)saveDiskIndexCount, GL_UNSIGNED_INT, 0);
            
            lightingShader.setVec3("objectColor", 1.0f, 1.0f, 1.0f);
            lightingShader.setFloat("emissiveStrength", 0.0f);

            // 5. Gestión del Menú de Guardado superpuesto (UI / HUD)
            float saveDistance = glm::length(glm::vec2(playerPosition.x - savePointPosition.x, playerPosition.z - savePointPosition.z));
            bool savePointInRange = saveDistance <= SAVE_POINT_INTERACT_RADIUS;

            if (saveMenuOpen || savePointInRange) 
            {
                // Configuración común para pintar elementos 2D superpuestos
                glDisable(GL_DEPTH_TEST);
                glDisable(GL_CULL_FACE);
                
                lightingShader.use();
                lightingShader.setMat4("projection", glm::ortho(0.0f, static_cast<float>(SCR_WIDTH), static_cast<float>(SCR_HEIGHT), 0.0f, -1.0f, 1.0f));
                lightingShader.setMat4("view", glm::mat4(1.0f));
                lightingShader.setVec3("viewPos", 0.0f, 0.0f, 1.0f);
                lightingShader.setVec3("dirLight_direction", 0.0f, 0.0f, -1.0f);
                lightingShader.setVec3("dirLight_color", 1.0f, 1.0f, 1.0f);
                lightingShader.setInt("numPointLights", 0);
                lightingShader.setInt("fogEnabled", 0);
                lightingShader.setInt("useSkinning", 0);
                lightingShader.setFloat("material_ambientStrength", 1.0f);
                lightingShader.setFloat("material_specularStrength", 0.0f);
                lightingShader.setFloat("emissiveStrength", 0.0f);
                lightingShader.setFloat("alphaCutoff", 0.01f);

                if (saveMenuOpen) 
                {
                    // Forzar cursor libre para seleccionar ranuras si fuera necesario
                    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);

                    // Capas de fondo rojo translúcido y contenedores estilizados
                    drawHudQuad(getWhiteTexture(), 0.0f, 0.0f, static_cast<float>(SCR_WIDTH), static_cast<float>(SCR_HEIGHT), glm::vec3(0.11f, 0.0f, 0.0f), 0.92f);
                    drawHudQuad(getWhiteTexture(), 74.0f, 78.0f, 690.0f, 8.0f, glm::vec3(0.95f, 0.05f, 0.04f), 0.50f);
                    drawHudQuad(getWhiteTexture(), 100.0f, 96.0f, 610.0f, 90.0f, glm::vec3(0.72f, 0.03f, 0.03f), 0.38f);
                    drawHudQuad(getWhiteTexture(), 52.0f, 210.0f, 900.0f, 92.0f, glm::vec3(0.20f, 0.0f, 0.0f), 0.55f);
                    drawHudQuad(getWhiteTexture(), 54.0f, 208.0f, 896.0f, 2.0f, glm::vec3(0.95f, 0.09f, 0.08f), 0.36f);
                    drawHudQuad(getWhiteTexture(), 62.0f, 219.0f, 78.0f, 48.0f, glm::vec3(0.28f, 0.07f, 0.06f), 0.95f);
                    drawHudQuad(getWhiteTexture(), 86.0f, 238.0f, 30.0f, 10.0f, glm::vec3(1.0f, 0.02f, 0.01f), 0.80f);
                    drawHudQuad(getWhiteTexture(), 690.0f, 0.0f, 420.0f, static_cast<float>(SCR_HEIGHT), glm::vec3(0.34f, 0.0f, 0.0f), 0.12f);

                    // Renderizado del retrato 3D estilizado de James en el menú de guardado
                    if (!jamesMeshes.empty()) {
                        glEnable(GL_DEPTH_TEST);
                        glClear(GL_DEPTH_BUFFER_BIT);
                        glViewport(SCR_WIDTH / 2, 0, SCR_WIDTH / 2, SCR_HEIGHT);
                        
                        glm::mat4 portraitProjection = glm::perspective(glm::radians(22.0f), (float)(SCR_WIDTH / 2) / (float)SCR_HEIGHT, 0.1f, 20.0f);
                        glm::mat4 portraitView = glm::lookAt(glm::vec3(0.0f, 2.35f, 3.35f), glm::vec3(0.0f, 2.35f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
                        
                        glm::vec3 jamesCenter = (jamesAABBMin + jamesAABBMax) * 0.5f;
                        float jamesMinY = jamesAABBMin.y;
                        glm::mat4 portraitModel = glm::mat4(1.0f);
                        portraitModel = glm::translate(portraitModel, glm::vec3(0.18f, 0.0f, 0.0f));
                        portraitModel = glm::rotate(portraitModel, glm::radians(MODEL_ROT_X), glm::vec3(1.0f, 0.0f, 0.0f));
                        portraitModel = glm::rotate(portraitModel, glm::radians(MODEL_ROT_Y), glm::vec3(0.0f, 1.0f, 0.0f));
                        portraitModel = glm::rotate(portraitModel, glm::radians(MODEL_ROT_Z), glm::vec3(0.0f, 0.0f, 1.0f));
                        portraitModel = glm::scale(portraitModel, glm::vec3(jamesRenderScale * 1.45f));
                        portraitModel = glm::translate(portraitModel, glm::vec3(-jamesCenter.x, -jamesMinY, -jamesCenter.z));

                        lightingShader.setMat4("projection", portraitProjection);
                        lightingShader.setMat4("view", portraitView);
                        lightingShader.setMat4("model", portraitModel);
                        lightingShader.setVec3("viewPos", 0.0f, 3.15f, 4.0f);
                        lightingShader.setVec3("dirLight_direction", -0.25f, -0.25f, -1.0f);
                        lightingShader.setVec3("dirLight_color", 1.0f, 0.10f, 0.08f);
                        lightingShader.setFloat("material_ambientStrength", 0.92f);
                        lightingShader.setFloat("material_specularStrength", 0.04f);
                        lightingShader.setFloat("objectAlpha", 0.55f);
                        lightingShader.setFloat("emissiveStrength", 0.04f);
                        lightingShader.setInt("useSkinning", jamesBoneCount > 0 && jamesAnimState.current ? 1 : 0);
                        
                        for (int i = 0; i < glm::min(jamesBoneCount, MAX_BONES); ++i) {
                            lightingShader.setMat4("finalBonesMatrices[" + std::to_string(i) + "]", jamesAnimState.finalMatrices[i]);
                        }
                        for (auto &m : jamesMeshes) {
                            lightingShader.setVec3("objectColor", glm::vec3(1.0f, 0.18f, 0.12f) * m.materialColor);
                            glActiveTexture(GL_TEXTURE0);
                            glBindTexture(GL_TEXTURE_2D, m.texture);
                            glBindVertexArray(m.VAO);
                            glDrawElements(GL_TRIANGLES, (GLsizei)m.indexCount, GL_UNSIGNED_INT, 0);
                        }
                        
                        glDisable(GL_DEPTH_TEST);
                        glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT); // Restaurar Viewport completo
                        
                        // Re-vincular configuraciones Ortogonales para los textos del menú
                        lightingShader.setInt("useSkinning", 0);
                        lightingShader.setMat4("projection", glm::ortho(0.0f, static_cast<float>(SCR_WIDTH), static_cast<float>(SCR_HEIGHT), 0.0f, -1.0f, 1.0f));
                        lightingShader.setMat4("view", glm::mat4(1.0f));
                        lightingShader.setVec3("viewPos", 0.0f, 0.0f, 1.0f);
                        lightingShader.setVec3("dirLight_direction", 0.0f, 0.0f, -1.0f);
                        lightingShader.setVec3("dirLight_color", 1.0f, 1.0f, 1.0f);
                        lightingShader.setFloat("material_ambientStrength", 1.0f);
                        lightingShader.setFloat("material_specularStrength", 0.0f);
                        lightingShader.setFloat("objectAlpha", 1.0f);
                        lightingShader.setFloat("emissiveStrength", 0.0f);
                    }

                    // Renderizar las texturas de textos informativos en el menú de guardado
                    drawHudQuad(saveTitleText.texture, 64.0f, 28.0f, static_cast<float>(saveTitleText.width), static_cast<float>(saveTitleText.height), glm::vec3(1.0f), 1.0f);
                    drawHudQuad(saveEmptyText.texture, 190.0f, 98.0f, static_cast<float>(saveEmptyText.width), static_cast<float>(saveEmptyText.height), glm::vec3(1.0f), 1.0f);
                    drawHudQuad(saveLocationText.texture, 156.0f, 220.0f, static_cast<float>(saveLocationText.width), static_cast<float>(saveLocationText.height), glm::vec3(1.0f), 1.0f);
                    drawHudQuad(saveTimeText.texture, 500.0f, 220.0f, static_cast<float>(saveTimeText.width), static_cast<float>(saveTimeText.height), glm::vec3(1.0f), 1.0f);
                    drawHudQuad(savePromptText.texture, 948.0f, 660.0f, static_cast<float>(savePromptText.width), static_cast<float>(savePromptText.height), glm::vec3(1.0f), 1.0f);
                } 
                else if (savePointInRange) 
                {
                    // Si solo está cerca, muestra el indicador inferior derecho para interactuar (Letra E)
                    drawHudQuad(getWhiteTexture(), 628.0f, 628.0f, 120.0f, 32.0f, glm::vec3(0.12f, 0.0f, 0.0f), 0.48f);
                    drawHudQuad(getWhiteTexture(), 628.0f, 628.0f, 120.0f, 1.0f, glm::vec3(0.62f, 0.08f, 0.06f), 0.42f);
                    drawHudQuad(saveInteractText.texture, 645.0f, 630.0f, static_cast<float>(saveInteractText.width), static_cast<float>(saveInteractText.height), glm::vec3(1.0f), 1.0f);
                }
                
                // Limpieza de estados HUD volviendo a configuraciones de juego estándar
                lightingShader.setFloat("objectAlpha", 1.0f);
                lightingShader.setFloat("alphaCutoff", 0.38f);
                lightingShader.setFloat("material_ambientStrength", 0.48f);
                lightingShader.setFloat("material_specularStrength", 0.12f);
                lightingShader.setInt("fogEnabled", 1);
                glEnable(GL_DEPTH_TEST);
            }
        }

        // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
        // -------------------------------------------------------------------------------
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // optional: de-allocate model resources
    // ------------------------------------------------------------------------
    for (auto &m : jamesMeshes) {
        if (m.VAO) glDeleteVertexArrays(1, &m.VAO);
        if (m.VBO) glDeleteBuffers(1, &m.VBO);
        if (m.EBO) glDeleteBuffers(1, &m.EBO);
        if (m.texture) glDeleteTextures(1, &m.texture);
    }
    for (auto &m : mapMeshes) {
        if (m.VAO) glDeleteVertexArrays(1, &m.VAO);
        if (m.VBO) glDeleteBuffers(1, &m.VBO);
        if (m.EBO) glDeleteBuffers(1, &m.EBO);
        if (m.texture) glDeleteTextures(1, &m.texture);
    }
    shutdownAudio();
    Gdiplus::GdiplusShutdown(gdiplusToken);

    // glfw: terminate, clearing all previously allocated GLFW resources.
    // ------------------------------------------------------------------
    glfwTerminate();
    return 0;
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow *window)
{
    // Global inputs that work in all states
    if(currentState == MENU)
    {
        if(glfwGetKey(window, GLFW_KEY_ENTER) == GLFW_PRESS)
            currentState = PLAYING;

        if(glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
            glfwSetWindowShouldClose(window,true);

        return;
    }


    playerIsMoving = false;
    bool ePressed = glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS;
    bool escapePressed = glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS;
    if (saveMenuOpen) {
        if ((ePressed && !eWasPressed) || escapePressed) {
            saveMenuOpen = false;
        }
        eWasPressed = ePressed;
        return;
    }

    if (escapePressed)
        glfwSetWindowShouldClose(window, true);

    float saveDistance = glm::length(glm::vec2(playerPosition.x - savePointPosition.x, playerPosition.z - savePointPosition.z));
    if (ePressed && !eWasPressed && saveDistance <= SAVE_POINT_INTERACT_RADIUS) {
        saveMenuOpen = true;
        eWasPressed = ePressed;
        return;
    }
    eWasPressed = ePressed;

    bool spacePressed = glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS;
    if (spacePressed && !spaceWasPressed)
        jumpRequested = true;
    spaceWasPressed = spacePressed;

    glm::vec3 forward = glm::normalize(glm::vec3(camera.Front.x, 0.0f, camera.Front.z));
    glm::vec3 right = glm::normalize(glm::cross(forward, glm::vec3(0.0f, 1.0f, 0.0f)));
    glm::vec3 movement(0.0f);

    bool wPressed = glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS;
    bool sPressed = glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS;
    bool aPressed = glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS;
    bool dPressed = glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS;

    if (wPressed) movement += forward;
    if (sPressed) movement -= forward;
    if (aPressed && (wPressed || sPressed)) movement -= right;
    if (dPressed && (wPressed || sPressed)) movement += right;

    if (!wPressed && !sPressed) {
        float turnSpeed = 110.0f * deltaTime;
        if (aPressed && !dPressed) playerYaw -= turnSpeed;
        if (dPressed && !aPressed) playerYaw += turnSpeed;
    }

    if (glm::length(movement) > 0.001f) {
        movement = glm::normalize(movement);
        float speed = (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) ? 3.6f : 1.8f;
        glm::vec3 nextPosition = playerPosition + movement * speed * deltaTime;
        float nextGroundY = playerGroundY;
        if (findGroundHeightAt(nextPosition.x, nextPosition.z, playerGroundY + 0.75f, nextGroundY)) {
            playerGroundY = nextGroundY;
            nextPosition.y = playerGroundY;
            playerPosition = nextPosition;
            playerYaw = glm::degrees(atan2(movement.x, movement.z));
            playerIsMoving = true;
        }
    }

}

void updateThirdPersonCamera()
{
    glm::vec3 target = playerPosition + glm::vec3(0.0f, thirdPersonHeight, 0.0f);
    float yawRad = glm::radians(thirdPersonYaw);
    float pitchRad = glm::radians(thirdPersonPitch);

    glm::vec3 offset;
    offset.x = cosf(pitchRad) * cosf(yawRad) * thirdPersonDistance;
    offset.y = sinf(pitchRad) * thirdPersonDistance;
    offset.z = cosf(pitchRad) * sinf(yawRad) * thirdPersonDistance;

    glm::vec3 basePosition = target - offset;
    glm::vec3 baseFront = glm::normalize(target - basePosition);
    glm::vec3 shoulderRight = glm::normalize(glm::cross(baseFront, glm::vec3(0.0f, 1.0f, 0.0f)));

    camera.Position = basePosition + shoulderRight * thirdPersonShoulderOffset;
    glm::vec3 lookTarget = target + shoulderRight * thirdPersonLookOffset;
    camera.Front = glm::normalize(lookTarget - camera.Position);
    camera.Up = glm::vec3(0.0f, 1.0f, 0.0f);
    camera.Yaw = glm::degrees(atan2(camera.Front.z, camera.Front.x));
    camera.Pitch = glm::degrees(asinf(glm::clamp(camera.Front.y, -1.0f, 1.0f)));
    camera.Zoom = 42.0f;
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    // make sure the viewport matches the new window dimensions; note that width and 
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}


// glfw: whenever the mouse moves, this callback is called
// -------------------------------------------------------
void mouse_callback(GLFWwindow* window, double xposIn, double yposIn)
{
    float xpos = static_cast<float>(xposIn);
    float ypos = static_cast<float>(yposIn);
    if (firstMouse)
    {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top

    lastX = xpos;
    lastY = ypos;

    thirdPersonYaw += xoffset * 0.12f;
    thirdPersonPitch += yoffset * 0.08f;
    thirdPersonPitch = glm::clamp(thirdPersonPitch, -35.0f, 15.0f);
}

// glfw: whenever the mouse scroll wheel scrolls, this callback is called
// ----------------------------------------------------------------------
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    thirdPersonDistance -= static_cast<float>(yoffset) * 0.35f;
    thirdPersonDistance = glm::clamp(thirdPersonDistance, 1.9f, 5.8f);
}

bool audioCommand(const std::string& command)
{
    char errorText[256] = {};
    MCIERROR error = mciSendStringA(command.c_str(), nullptr, 0, nullptr);
    if (error != 0) {
        mciGetErrorStringA(error, errorText, sizeof(errorText));
        std::cout << "Audio command failed: " << command << " -> " << errorText << std::endl;
        return false;
    }
    return true;
}

void initAudio()
{
    audioCommand("close bgm");
    audioCommand("close footsteps");

    if (audioCommand("open \"sounds\\2-white-noiz.mp3\" type mpegvideo alias bgm")) {
        audioCommand("setaudio bgm volume to 95");
        audioCommand("play bgm repeat");
    }

    if (audioCommand("open \"sounds\\walking_soft.wav\" type waveaudio alias footsteps")) {
        audioCommand("setaudio footsteps volume to 1000");
    }
}

void updateFootstepAudio()
{
    if (playerIsMoving) {
        footstepTimer -= deltaTime;
        if (footstepTimer <= 0.0f) {
            audioCommand("stop footsteps");
            audioCommand("seek footsteps to start");
            if (audioCommand("play footsteps from 0")) {
                footstepsPlaying = true;
                footstepTimer = 0.60f;
            }
        }
    } else if (footstepsPlaying) {
        audioCommand("stop footsteps");
        audioCommand("seek footsteps to start");
        footstepsPlaying = false;
        footstepTimer = 0.0f;
    }
}

void shutdownAudio()
{
    audioCommand("stop footsteps");
    audioCommand("stop bgm");
    audioCommand("close footsteps");
    audioCommand("close bgm");
}


unsigned int loadTextureFromFile(const std::string &path, const std::string &directory, std::map<std::string,unsigned int> &loaded)
{
    std::string filename = path;
    // if path is relative, prepend directory
    std::filesystem::path inputPath(filename);
    if (filename.size() && !inputPath.is_absolute() && filename.find(":/") == std::string::npos && filename.find(":\\") == std::string::npos && (filename.find("\\") != 0)) {
        filename = directory + "/" + filename;
    }
    // if the file does not exist (e.g. absolute path to another machine), try to locate it in local assets
    if (!std::filesystem::exists(filename)) {
        std::cout << "Texture file not found at original path: " << filename << ". Will try to locate in assets/" << std::endl;
        auto base = std::filesystem::path(filename).filename().string();
        std::filesystem::path candidate1 = std::filesystem::path("assets") / "textures" / base;
        std::filesystem::path candidate2 = std::filesystem::path("assets") / base;
        bool found = false;
        if (std::filesystem::exists(candidate1)) {
            filename = candidate1.string();
            std::cout << "Found texture in: " << filename << std::endl;
            found = true;
        }
        else if (std::filesystem::exists(candidate2)) {
            filename = candidate2.string();
            std::cout << "Found texture in: " << filename << std::endl;
            found = true;
        } else {
            // recursive search in assets/ tree
            std::vector<std::filesystem::path> searchRoots;
            searchRoots.push_back(std::filesystem::path(directory));
            if (std::filesystem::path(directory).has_parent_path()) searchRoots.push_back(std::filesystem::path(directory).parent_path());
            searchRoots.push_back(std::filesystem::path("assets"));
            searchRoots.push_back(std::filesystem::path("..") / "assets");
            for (auto &root : searchRoots) {
                try {
                    if (!std::filesystem::exists(root)) continue;
                    for (auto& p : std::filesystem::recursive_directory_iterator(root)) {
                        if (!p.is_regular_file()) continue;
                        if (p.path().filename().string() == base) {
                            filename = p.path().string();
                            std::cout << "Found texture by recursive search: " << filename << std::endl;
                            found = true;
                            break;
                        }
                    }
                } catch (std::filesystem::filesystem_error &e) {
                    std::cout << "Filesystem error while searching " << root.string() << ": " << e.what() << std::endl;
                }
                if (found) break;
            }
            if (!found) std::cout << "Could not find texture '" << base << "' in assets/; using empty texture" << std::endl;
        }
    }
    if (loaded.find(filename) != loaded.end()) return loaded[filename];
    std::string ext = filename.size() >= 4 ? filename.substr(filename.find_last_of('.') + 1) : std::string();
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

    unsigned int tex = 0;
    if (ext != "tga" && ext != "ktx" && ext != "dds") {
        std::wstring wfn(filename.begin(), filename.end());
        tex = loadTextureFromJpeg(wfn.c_str());
    }
    if (tex == 0) {
        if (ext == "tga" || ext == "ktx" || ext == "dds") {
            stbi_set_flip_vertically_on_load(1);
            int w=0,h=0,n=0;
            unsigned char* data = stbi_load(filename.c_str(), &w, &h, &n, 4);
            if (data) {
                glGenTextures(1, &tex);
                glBindTexture(GL_TEXTURE_2D, tex);
                glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
                applyTextureParams(true);
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
                glGenerateMipmap(GL_TEXTURE_2D);
                stbi_image_free(data);
                std::cout << "Loaded texture via stb_image: " << filename << std::endl;
            } else {
                std::cout << "stb_image failed to load: " << filename << " (" << stbi_failure_reason() << ")" << std::endl;
            }
        }
        if (tex == 0) std::cout << "loadTextureFromFile: failed to create GL texture for '" << filename << "'" << std::endl;
    }
    loaded[filename] = tex;
    return tex;
}

unsigned int getWhiteTexture()
{
    static unsigned int whiteTex = 0;
    if (whiteTex) return whiteTex;
    unsigned char data[4] = { 255,255,255,255 };
    glGenTextures(1, &whiteTex);
    glBindTexture(GL_TEXTURE_2D, whiteTex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    return whiteTex;
}

unsigned int loadEmbeddedTexture(aiTexture* atex)
{
    unsigned int glt = 0;
    if (atex->mHeight == 0) {
        int w=0,h=0,n=0;
        unsigned char* img = stbi_load_from_memory((unsigned char*)atex->pcData, (int)atex->mWidth, &w, &h, &n, 4);
        if (img) {
            glGenTextures(1, &glt);
            glBindTexture(GL_TEXTURE_2D, glt);
            glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
            applyTextureParams(true);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, img);
            glGenerateMipmap(GL_TEXTURE_2D);
            stbi_image_free(img);
        }
    } else {
        int w = atex->mWidth;
        int h = atex->mHeight;
        std::vector<unsigned char> img(w * h * 4);
        aiTexel* src = reinterpret_cast<aiTexel*>(atex->pcData);
        for (int y = 0; y < h; ++y) {
            for (int x = 0; x < w; ++x) {
                aiTexel &t = src[y * w + x];
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
        applyTextureParams(true);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, img.data());
        glGenerateMipmap(GL_TEXTURE_2D);
    }
    return glt;
}

unsigned int loadMaterialTexture(aiMaterial* material, const aiScene* scene, const std::string& directory, std::map<std::string,unsigned int>& loaded)
{
    std::vector<aiTextureType> textureTypes = {
        aiTextureType_BASE_COLOR,
        aiTextureType_DIFFUSE,
        aiTextureType_UNKNOWN
    };

    for (aiTextureType type : textureTypes) {
        if (material->GetTextureCount(type) == 0) continue;
        aiString str;
        if (material->GetTexture(type, 0, &str) != AI_SUCCESS) continue;
        std::string texPath = str.C_Str();
        if (texPath.empty()) continue;

        if (texPath[0] == '*') {
            int texIndex = atoi(texPath.c_str() + 1);
            if (texIndex >= 0 && scene->mNumTextures > (unsigned)texIndex) {
                return loadEmbeddedTexture(scene->mTextures[texIndex]);
            }
        } else {
            unsigned int texture = loadTextureFromFile(texPath, directory, loaded);
            if (texture != 0) return texture;
        }
    }

    if (scene->mNumTextures > 0) {
        unsigned int textureIndex = material->GetName().length % scene->mNumTextures;
        return loadEmbeddedTexture(scene->mTextures[textureIndex]);
    }

    return 0;
}

std::vector<unsigned int> loadMaterialTextureFallbacks(const std::string& path, std::unordered_map<std::string, unsigned int>& byName)
{
    std::vector<unsigned int> textures;
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(path, aiProcess_Triangulate);
    if (!scene) {
        std::cout << "Fallback texture source failed: " << path << " " << importer.GetErrorString() << std::endl;
        return textures;
    }

    std::string directory = ".";
    size_t pos = path.find_last_of("/\\");
    if (pos != std::string::npos) directory = path.substr(0, pos);

    std::map<std::string,unsigned int> loaded;
    for (unsigned int i = 0; i < scene->mNumMaterials; ++i) {
        unsigned int tex = loadMaterialTexture(scene->mMaterials[i], scene, directory, loaded);
        if (tex == 0) tex = getWhiteTexture();
        textures.push_back(tex);
        std::string name = scene->mMaterials[i]->GetName().C_Str();
        std::transform(name.begin(), name.end(), name.begin(), ::tolower);
        if (!name.empty()) byName[name] = tex;
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
    if (generateMipmaps) {
        const char* extensions = reinterpret_cast<const char*>(glGetString(GL_EXTENSIONS));
        if (extensions && std::strstr(extensions, "GL_EXT_texture_filter_anisotropic")) {
            GLfloat maxAniso = 0.0f;
            glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &maxAniso);
            glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, glm::min(maxAniso, 8.0f));
        }
    }
}

std::vector<WalkTriangle> buildWalkTriangles(const std::vector<MeshData>& meshes, const glm::mat4& modelTransform)
{
    std::vector<WalkTriangle> triangles;
    for (const auto& mesh : meshes) {
        for (size_t i = 0; i + 2 < mesh.indices.size(); i += 3) {
            glm::vec3 a = glm::vec3(modelTransform * glm::vec4(mesh.positions[mesh.indices[i]], 1.0f));
            glm::vec3 b = glm::vec3(modelTransform * glm::vec4(mesh.positions[mesh.indices[i + 1]], 1.0f));
            glm::vec3 c = glm::vec3(modelTransform * glm::vec4(mesh.positions[mesh.indices[i + 2]], 1.0f));
            glm::vec3 normal = glm::cross(b - a, c - a);
            float area = glm::length(normal);
            if (area < 0.0001f) continue;
            normal = glm::normalize(normal);
            if (normal.y < 0.45f) continue;

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

glm::vec3 findSpawnPoint()
{
    if (walkTriangles.empty()) return glm::vec3(0.0f);

    float lowestY = FLT_MAX;
    for (const auto& tri : walkTriangles) {
        glm::vec3 center = (tri.a + tri.b + tri.c) / 3.0f;
        lowestY = glm::min(lowestY, center.y);
    }

    const WalkTriangle* best = nullptr;
    float bestScore = -FLT_MAX;
    for (const auto& tri : walkTriangles) {
        glm::vec3 center = (tri.a + tri.b + tri.c) / 3.0f;
        if (center.y > lowestY + 1.2f) continue;
        float distanceFromOrigin = glm::length(glm::vec2(center.x, center.z));
        float score = tri.area - distanceFromOrigin * 0.05f;
        if (score > bestScore) {
            bestScore = score;
            best = &tri;
        }
    }

    if (!best) {
        for (const auto& tri : walkTriangles) {
            glm::vec3 center = (tri.a + tri.b + tri.c) / 3.0f;
            float score = -center.y;
            if (score > bestScore) {
                bestScore = score;
                best = &tri;
            }
        }
    }

    glm::vec3 spawn = (best->a + best->b + best->c) / 3.0f;
    spawn.y += 0.02f;
    return spawn;
}

bool findGroundHeightAt(float x, float z, float maxStepUp, float& outY)
{
    float bestY = -FLT_MAX;
    for (const auto& tri : walkTriangles) {
        if (x < tri.minX || x > tri.maxX || z < tri.minZ || z > tri.maxZ) continue;

        glm::vec2 a(tri.a.x, tri.a.z);
        glm::vec2 b(tri.b.x, tri.b.z);
        glm::vec2 c(tri.c.x, tri.c.z);
        glm::vec2 p(x, z);
        glm::vec2 v0 = b - a;
        glm::vec2 v1 = c - a;
        glm::vec2 v2 = p - a;
        float den = v0.x * v1.y - v1.x * v0.y;
        if (fabsf(den) < 0.000001f) continue;

        float u = (v2.x * v1.y - v1.x * v2.y) / den;
        float v = (v0.x * v2.y - v2.x * v0.y) / den;
        float w = 1.0f - u - v;
        if (u >= -0.001f && v >= -0.001f && w >= -0.001f) {
            float y = w * tri.a.y + u * tri.b.y + v * tri.c.y;
            if (y > bestY && y <= maxStepUp) {
                bestY = y;
            }
        }
    }
    if (bestY > -FLT_MAX * 0.5f) {
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

glm::vec3 closestPointOnTriangle(const glm::vec3& p, const glm::vec3& a, const glm::vec3& b, const glm::vec3& c)
{
    glm::vec3 ab = b - a;
    glm::vec3 ac = c - a;
    glm::vec3 ap = p - a;
    float d1 = glm::dot(ab, ap);
    float d2 = glm::dot(ac, ap);
    if (d1 <= 0.0f && d2 <= 0.0f) return a;

    glm::vec3 bp = p - b;
    float d3 = glm::dot(ab, bp);
    float d4 = glm::dot(ac, bp);
    if (d3 >= 0.0f && d4 <= d3) return b;

    float vc = d1 * d4 - d3 * d2;
    if (vc <= 0.0f && d1 >= 0.0f && d3 <= 0.0f) {
        float v = d1 / (d1 - d3);
        return a + v * ab;
    }

    glm::vec3 cp = p - c;
    float d5 = glm::dot(ab, cp);
    float d6 = glm::dot(ac, cp);
    if (d6 >= 0.0f && d5 <= d6) return c;

    float vb = d5 * d2 - d1 * d6;
    if (vb <= 0.0f && d2 >= 0.0f && d6 <= 0.0f) {
        float w = d2 / (d2 - d6);
        return a + w * ac;
    }

    float va = d3 * d6 - d5 * d4;
    if (va <= 0.0f && (d4 - d3) >= 0.0f && (d5 - d6) >= 0.0f) {
        float w = (d4 - d3) / ((d4 - d3) + (d5 - d6));
        return b + w * (c - b);
    }

    float denom = 1.0f / (va + vb + vc);
    float v = vb * denom;
    float w = vc * denom;
    return a + ab * v + ac * w;
}

bool findWallAttachment(const std::vector<MeshData>& meshes, const glm::mat4& modelTransform, const glm::vec3& anchor, const glm::vec3& preferredNormal, glm::vec3& outPosition, glm::vec3& outNormal)
{
    bool found = false;
    float bestScore = FLT_MAX;
    const float targetHeight = anchor.y;
    const float minWallHeight = 1.15f;
    glm::vec3 preferred = glm::normalize(preferredNormal);

    for (const auto& mesh : meshes) {
        for (size_t i = 0; i + 2 < mesh.indices.size(); i += 3) {
            glm::vec3 a = glm::vec3(modelTransform * glm::vec4(mesh.positions[mesh.indices[i]], 1.0f));
            glm::vec3 b = glm::vec3(modelTransform * glm::vec4(mesh.positions[mesh.indices[i + 1]], 1.0f));
            glm::vec3 c = glm::vec3(modelTransform * glm::vec4(mesh.positions[mesh.indices[i + 2]], 1.0f));
            glm::vec3 normal = glm::cross(b - a, c - a);
            float doubleArea = glm::length(normal);
            if (doubleArea < 0.12f) continue;

            normal = glm::normalize(normal);
            if (fabsf(normal.y) > 0.25f) continue;

            glm::vec3 closest = closestPointOnTriangle(anchor, a, b, c);
            if (closest.y < minWallHeight) continue;

            float distance = glm::length(closest - anchor);
            if (distance > 6.0f) continue;

            float heightError = fabsf(closest.y - targetHeight);
            glm::vec3 outwardNormal = glm::dot(normal, preferred) < 0.0f ? -normal : normal;
            float normalMismatch = 1.0f - glm::max(glm::dot(outwardNormal, preferred), 0.0f);
            if (normalMismatch > 0.55f) continue;

            float score = distance + heightError * 4.0f + normalMismatch * 8.0f - glm::min(doubleArea, 3.0f) * 0.08f;
            if (score < bestScore) {
                bestScore = score;
                outNormal = outwardNormal;
                outPosition = closest + outNormal * 0.018f;
                found = true;
            }
        }
    }

    return found;
}

glm::mat4 makeWallModel(const glm::vec3& position, const glm::vec3& normal, const glm::vec3& scale)
{
    glm::vec3 n = glm::normalize(normal);
    glm::vec3 up(0.0f, 1.0f, 0.0f);
    glm::vec3 right = glm::normalize(glm::cross(up, n));
    if (glm::length(right) < 0.001f) right = glm::vec3(1.0f, 0.0f, 0.0f);
    glm::vec3 localUp = glm::normalize(glm::cross(n, right));

    glm::mat4 basis(1.0f);
    basis[0] = glm::vec4(right, 0.0f);
    basis[1] = glm::vec4(localUp, 0.0f);
    basis[2] = glm::vec4(n, 0.0f);

    return glm::translate(glm::mat4(1.0f), position) * basis * glm::scale(glm::mat4(1.0f), scale);
}

void createCube(unsigned int &VAO, unsigned int &indexCount)
{
    float vertices[] = {
        -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
         0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
         0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
        -0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
        -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
         0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
        -0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
        -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,
        -0.5f,  0.5f, -0.5f, -1.0f,  0.0f,  0.0f,
        -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,
        -0.5f, -0.5f,  0.5f, -1.0f,  0.0f,  0.0f,
         0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,
         0.5f,  0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
         0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
         0.5f, -0.5f,  0.5f,  1.0f,  0.0f,  0.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,
         0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,
         0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,
        -0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,
        -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,
         0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
        -0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
    };
    unsigned int indices[] = {
        0, 1, 2, 2, 3, 0,
        4, 6, 5, 4, 7, 6,
        8, 9, 10, 10, 11, 8,
        12, 14, 13, 12, 15, 14,
        16, 18, 17, 16, 19, 18,
        20, 21, 22, 22, 23, 20,
    };

    indexCount = 36;
    unsigned int VBO, EBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindVertexArray(0);
}

void createQuad(unsigned int &VAO, unsigned int &indexCount)
{
    float vertices[] = {
        -0.5f, -0.5f, 0.0f,  0.0f, 0.0f, 1.0f,  0.0f, 0.0f,
         0.5f, -0.5f, 0.0f,  0.0f, 0.0f, 1.0f,  1.0f, 0.0f,
         0.5f,  0.5f, 0.0f,  0.0f, 0.0f, 1.0f,  1.0f, 1.0f,
        -0.5f,  0.5f, 0.0f,  0.0f, 0.0f, 1.0f,  0.0f, 1.0f,
    };
    unsigned int indices[] = {
        0, 1, 2,
        2, 3, 0,
    };

    indexCount = 6;
    unsigned int VBO, EBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);
    glBindVertexArray(0);
}

void createDisk(unsigned int &VAO, unsigned int &indexCount)
{
    constexpr unsigned int segments = 32;
    std::vector<float> vertices;
    std::vector<unsigned int> indices;

    vertices.insert(vertices.end(), {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.5f, 0.5f});
    for (unsigned int i = 0; i <= segments; ++i) {
        float angle = (float)i / (float)segments * 6.28318530718f;
        float x = cosf(angle) * 0.5f;
        float y = sinf(angle) * 0.5f;
        vertices.insert(vertices.end(), {x, y, 0.0f, 0.0f, 0.0f, 1.0f, x + 0.5f, y + 0.5f});
    }

    for (unsigned int i = 1; i <= segments; ++i) {
        indices.push_back(0);
        indices.push_back(i);
        indices.push_back(i + 1);
    }

    indexCount = static_cast<unsigned int>(indices.size());
    unsigned int VBO, EBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);
    glBindVertexArray(0);
}

MeshData processMesh(aiMesh* mesh, const aiScene* scene, const glm::mat4& transform, const std::string &directory, glm::vec3 &aabbMin, glm::vec3 &aabbMax, std::map<std::string,unsigned int> &loaded, std::unordered_map<std::string, BoneInfo>* boneInfoMap, int* boneCounter)
{
    std::vector<float> vertices;
    std::vector<unsigned int> indices;
    MeshData mdata;
    glm::mat3 normalTransform = glm::transpose(glm::inverse(glm::mat3(transform)));
    std::vector<BoneVertexData> boneData(mesh->mNumVertices);
    for (auto& b : boneData) {
        b.ids.fill(-1);
        b.weights.fill(0.0f);
    }
    if (boneInfoMap && boneCounter && mesh->HasBones()) {
        mdata.hasBones = true;
        for (unsigned int boneIndex = 0; boneIndex < mesh->mNumBones; ++boneIndex) {
            aiBone* bone = mesh->mBones[boneIndex];
            std::string boneName = bone->mName.C_Str();
            int boneID = -1;
            auto found = boneInfoMap->find(boneName);
            if (found == boneInfoMap->end()) {
                if (*boneCounter >= MAX_BONES) continue;
                BoneInfo info;
                info.id = *boneCounter;
                info.offset = aiToGlm(bone->mOffsetMatrix);
                (*boneInfoMap)[boneName] = info;
                boneID = *boneCounter;
                ++(*boneCounter);
            } else {
                boneID = found->second.id;
            }

            for (unsigned int weightIndex = 0; weightIndex < bone->mNumWeights; ++weightIndex) {
                unsigned int vertexId = bone->mWeights[weightIndex].mVertexId;
                float weight = bone->mWeights[weightIndex].mWeight;
                if (vertexId >= boneData.size()) continue;
                for (int slot = 0; slot < MAX_BONE_INFLUENCE; ++slot) {
                    if (boneData[vertexId].ids[slot] < 0) {
                        boneData[vertexId].ids[slot] = boneID;
                        boneData[vertexId].weights[slot] = weight;
                        break;
                    }
                }
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
        // update AABB
        aabbMin.x = std::min(aabbMin.x, pos.x);
        aabbMin.y = std::min(aabbMin.y, pos.y);
        aabbMin.z = std::min(aabbMin.z, pos.z);
        aabbMax.x = std::max(aabbMax.x, pos.x);
        aabbMax.y = std::max(aabbMax.y, pos.y);
        aabbMax.z = std::max(aabbMax.z, pos.z);
        // normal
        if (mesh->HasNormals()) {
            glm::vec3 normal = glm::normalize(vertexNormalTransform * glm::vec3(mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z));
            vertices.push_back(normal.x);
            vertices.push_back(normal.y);
            vertices.push_back(normal.z);
        } else {
            vertices.push_back(0.0f); vertices.push_back(0.0f); vertices.push_back(0.0f);
        }
        // texcoords
        if (mesh->mTextureCoords[0]) {
            vertices.push_back(mesh->mTextureCoords[0][i].x);
            vertices.push_back(mesh->mTextureCoords[0][i].y);
        } else {
            vertices.push_back(0.0f); vertices.push_back(0.0f);
        }
        for (int slot = 0; slot < MAX_BONE_INFLUENCE; ++slot) {
            vertices.push_back(static_cast<float>(boneData[i].ids[slot]));
        }
        for (int slot = 0; slot < MAX_BONE_INFLUENCE; ++slot) {
            vertices.push_back(boneData[i].weights[slot]);
        }
    }
    for (unsigned int i = 0; i < mesh->mNumFaces; ++i)
    {
        aiFace face = mesh->mFaces[i];
        for (unsigned int j = 0; j < face.mNumIndices; ++j) {
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
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, vertexStride, (void*)0);
    glEnableVertexAttribArray(0);
    // normal
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, vertexStride, (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    // tex
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, vertexStride, (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, vertexStride, (void*)(8 * sizeof(float)));
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, vertexStride, (void*)((8 + MAX_BONE_INFLUENCE) * sizeof(float)));
    glEnableVertexAttribArray(4);

    glBindVertexArray(0);

    // load material textures (diffuse), including embedded textures in glTF/.glb
    unsigned int texid = 0;
    if (mesh->mMaterialIndex >= 0) {
        aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
        aiColor4D diffuseColor(1.0f, 1.0f, 1.0f, 1.0f);
        if (AI_SUCCESS == aiGetMaterialColor(material, AI_MATKEY_COLOR_DIFFUSE, &diffuseColor)) {
            mdata.materialColor = glm::vec3(diffuseColor.r, diffuseColor.g, diffuseColor.b);
            mdata.materialAlpha = diffuseColor.a;
        }
        float opacity = 1.0f;
        if (AI_SUCCESS == material->Get(AI_MATKEY_OPACITY, opacity)) {
            mdata.materialAlpha *= opacity;
        }
        texid = loadMaterialTexture(material, scene, directory, loaded);
        if (texid == 0 && boneInfoMap && !jamesFallbackTextures.empty()) {
            std::string materialName = material->GetName().C_Str();
            std::string lowerName = materialName;
            std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);
            auto namedFallback = jamesFallbackTexturesByName.find(lowerName);
            if (namedFallback != jamesFallbackTexturesByName.end()) {
                texid = namedFallback->second;
            } else {
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
    if (mdata.texture == 0) mdata.texture = getWhiteTexture();
    return mdata;
}

glm::mat4 aiToGlm(const aiMatrix4x4& from)
{
    return glm::mat4(
        from.a1, from.b1, from.c1, from.d1,
        from.a2, from.b2, from.c2, from.d2,
        from.a3, from.b3, from.c3, from.d3,
        from.a4, from.b4, from.c4, from.d4
    );
}

void processNode(const aiScene* scene, aiNode* node, const glm::mat4& parentTransform, const std::string &directory, std::vector<MeshData> &meshes, glm::vec3 &aabbMin, glm::vec3 &aabbMax, std::map<std::string,unsigned int> &loaded, std::unordered_map<std::string, BoneInfo>* boneInfoMap, int* boneCounter)
{
    glm::mat4 nodeTransform = parentTransform * aiToGlm(node->mTransformation);
    for (unsigned int i = 0; i < node->mNumMeshes; ++i)
    {
        aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
        meshes.push_back(processMesh(mesh, scene, nodeTransform, directory, aabbMin, aabbMax, loaded, boneInfoMap, boneCounter));
    }
    for (unsigned int i = 0; i < node->mNumChildren; ++i)
        processNode(scene, node->mChildren[i], nodeTransform, directory, meshes, aabbMin, aabbMax, loaded, boneInfoMap, boneCounter);
}

std::vector<MeshData> loadModel(const std::string &path, glm::vec3 &outAABBMin, glm::vec3 &outAABBMax, std::unordered_map<std::string, BoneInfo>* boneInfoMap, int* boneCounter, glm::mat4* outGlobalInverse)
{
    Assimp::Importer importer;
    std::string ext = std::filesystem::path(path).extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    unsigned int importFlags = aiProcess_Triangulate |
                               aiProcess_GenSmoothNormals |
                               aiProcess_JoinIdenticalVertices |
                               aiProcess_ImproveCacheLocality;
    if (ext != ".dae") {
        importFlags |= aiProcess_FlipUVs;
    }
    const aiScene* scene = importer.ReadFile(path, importFlags);
    std::vector<MeshData> meshes;
    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
        std::cerr << "ERROR::ASSIMP::" << importer.GetErrorString() << std::endl;
        return meshes;
    }
    if (outGlobalInverse) {
        *outGlobalInverse = glm::inverse(aiToGlm(scene->mRootNode->mTransformation));
    }
    // get directory
    std::string directory = path;
    size_t pos = directory.find_last_of("/\\");
    if (pos != std::string::npos) directory = directory.substr(0, pos);
    else directory = ".";
    std::map<std::string,unsigned int> loaded;
    processNode(scene, scene->mRootNode, glm::mat4(1.0f), directory, meshes, outAABBMin, outAABBMax, loaded, boneInfoMap, boneCounter);
    return meshes;
}

AnimNode readAnimHierarchy(aiNode* node)
{
    AnimNode data;
    data.name = node->mName.C_Str();
    data.transform = aiToGlm(node->mTransformation);
    data.children.reserve(node->mNumChildren);
    for (unsigned int i = 0; i < node->mNumChildren; ++i) {
        data.children.push_back(readAnimHierarchy(node->mChildren[i]));
    }
    return data;
}

AnimationClip loadAnimationClip(const std::string& path, const std::string& name)
{
    AnimationClip clip;
    clip.name = name;

    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_LimitBoneWeights);
    if (!scene || !scene->mRootNode || scene->mNumAnimations == 0) {
        std::cout << "Animation load failed: " << path << " " << importer.GetErrorString() << std::endl;
        return clip;
    }

    aiAnimation* anim = scene->mAnimations[0];
    clip.duration = static_cast<float>(anim->mDuration);
    clip.ticksPerSecond = anim->mTicksPerSecond != 0.0 ? static_cast<float>(anim->mTicksPerSecond) : 25.0f;
    clip.root = readAnimHierarchy(scene->mRootNode);

    for (unsigned int i = 0; i < anim->mNumChannels; ++i) {
        aiNodeAnim* channel = anim->mChannels[i];
        AnimChannel out;
        out.name = channel->mNodeName.C_Str();
        out.positions.reserve(channel->mNumPositionKeys);
        out.rotations.reserve(channel->mNumRotationKeys);
        out.scales.reserve(channel->mNumScalingKeys);

        for (unsigned int key = 0; key < channel->mNumPositionKeys; ++key) {
            aiVector3D v = channel->mPositionKeys[key].mValue;
            out.positions.push_back({glm::vec3(v.x, v.y, v.z), static_cast<float>(channel->mPositionKeys[key].mTime)});
        }
        for (unsigned int key = 0; key < channel->mNumRotationKeys; ++key) {
            aiQuaternion q = channel->mRotationKeys[key].mValue;
            out.rotations.push_back({glm::normalize(glm::quat(q.w, q.x, q.y, q.z)), static_cast<float>(channel->mRotationKeys[key].mTime)});
        }
        for (unsigned int key = 0; key < channel->mNumScalingKeys; ++key) {
            aiVector3D v = channel->mScalingKeys[key].mValue;
            out.scales.push_back({glm::vec3(v.x, v.y, v.z), static_cast<float>(channel->mScalingKeys[key].mTime)});
        }
        clip.channels[out.name] = out;
    }

    clip.valid = true;
    return clip;
}

template <typename KeyT>
int getKeyIndex(const std::vector<KeyT>& keys, float animationTime)
{
    for (int i = 0; i + 1 < static_cast<int>(keys.size()); ++i) {
        if (animationTime < keys[i + 1].timeStamp) return i;
    }
    return glm::max(0, static_cast<int>(keys.size()) - 2);
}

float getScaleFactor(float lastTime, float nextTime, float animationTime)
{
    float span = nextTime - lastTime;
    if (fabsf(span) < 0.00001f) return 0.0f;
    return glm::clamp((animationTime - lastTime) / span, 0.0f, 1.0f);
}

glm::mat4 interpolateChannelTransform(const AnimChannel& channel, float animationTime)
{
    glm::vec3 translation(0.0f);
    glm::quat rotation(1.0f, 0.0f, 0.0f, 0.0f);
    glm::vec3 scale(1.0f);

    if (channel.positions.size() == 1) {
        translation = channel.positions[0].position;
    } else if (channel.positions.size() > 1) {
        int index = getKeyIndex(channel.positions, animationTime);
        int next = index + 1;
        float factor = getScaleFactor(channel.positions[index].timeStamp, channel.positions[next].timeStamp, animationTime);
        translation = glm::mix(channel.positions[index].position, channel.positions[next].position, factor);
    }

    if (channel.rotations.size() == 1) {
        rotation = channel.rotations[0].orientation;
    } else if (channel.rotations.size() > 1) {
        int index = getKeyIndex(channel.rotations, animationTime);
        int next = index + 1;
        float factor = getScaleFactor(channel.rotations[index].timeStamp, channel.rotations[next].timeStamp, animationTime);
        rotation = glm::normalize(glm::slerp(channel.rotations[index].orientation, channel.rotations[next].orientation, factor));
    }

    if (channel.scales.size() == 1) {
        scale = channel.scales[0].scale;
    } else if (channel.scales.size() > 1) {
        int index = getKeyIndex(channel.scales, animationTime);
        int next = index + 1;
        float factor = getScaleFactor(channel.scales[index].timeStamp, channel.scales[next].timeStamp, animationTime);
        scale = glm::mix(channel.scales[index].scale, channel.scales[next].scale, factor);
    }

    return glm::translate(glm::mat4(1.0f), translation) * glm::toMat4(rotation) * glm::scale(glm::mat4(1.0f), scale);
}

glm::mat4 interpolateChannelTransformNoTranslation(const AnimChannel& channel, float animationTime)
{
    glm::mat4 transform = interpolateChannelTransform(channel, animationTime);
    transform[3] = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
    return transform;
}

bool shouldStripTranslation(const std::string& nodeName)
{
    std::string lower = nodeName;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
    return lower == "root" ||
           lower.find("armature") != std::string::npos ||
           lower.find("hips") != std::string::npos ||
           lower.find("pelvis") != std::string::npos ||
           lower.find("mixamorig:hips") != std::string::npos;
}

void calculateBoneTransforms(const AnimNode& node, const glm::mat4& parentTransform, const AnimationClip& clip, std::vector<glm::mat4>& finalMatrices, const std::unordered_map<std::string, BoneInfo>& boneInfoMap)
{
    glm::mat4 nodeTransform = node.transform;
    auto channel = clip.channels.find(node.name);
    if (channel != clip.channels.end()) {
        nodeTransform = interpolateChannelTransform(channel->second, clip.duration > 0.0f ? fmodf(clip.duration, clip.duration) : 0.0f);
    }

    glm::mat4 globalTransform = parentTransform * nodeTransform;
    auto bone = boneInfoMap.find(node.name);
    if (bone != boneInfoMap.end() && bone->second.id < MAX_BONES) {
        finalMatrices[bone->second.id] = globalTransform * bone->second.offset;
    }

    for (const auto& child : node.children) {
        calculateBoneTransforms(child, globalTransform, clip, finalMatrices, boneInfoMap);
    }
}

void calculateBoneTransformsAtTime(const AnimNode& node, const glm::mat4& parentTransform, const AnimationClip& clip, float animationTime, std::vector<glm::mat4>& finalMatrices, const std::unordered_map<std::string, BoneInfo>& boneInfoMap, const glm::mat4& globalInverseTransform, bool isRoot)
{
    glm::mat4 nodeTransform = node.transform;
    auto channel = clip.channels.find(node.name);
    if (channel != clip.channels.end()) {
        nodeTransform = (isRoot || shouldStripTranslation(node.name))
            ? interpolateChannelTransformNoTranslation(channel->second, animationTime)
            : interpolateChannelTransform(channel->second, animationTime);
    }

    glm::mat4 globalTransform = parentTransform * nodeTransform;
    auto bone = boneInfoMap.find(node.name);
    if (bone != boneInfoMap.end() && bone->second.id < MAX_BONES) {
        finalMatrices[bone->second.id] = globalInverseTransform * globalTransform * bone->second.offset;
    }

    for (const auto& child : node.children) {
        calculateBoneTransformsAtTime(child, globalTransform, clip, animationTime, finalMatrices, boneInfoMap, globalInverseTransform, false);
    }
}

void updateAnimation(AnimationState& state, const AnimationClip* clip, float deltaSeconds, const std::unordered_map<std::string, BoneInfo>& boneInfoMap, int boneCount, const glm::mat4& globalInverseTransform, bool looping)
{
    if (!clip || !clip->valid || boneCount <= 0) {
        state.finalMatrices.assign(MAX_BONES, glm::mat4(1.0f));
        state.current = nullptr;
        state.currentTime = 0.0f;
        return;
    }

    if (state.current != clip) {
        state.current = clip;
        state.currentTime = 0.0f;
        state.currentLooping = looping;
    }
    state.currentTime += clip->ticksPerSecond * deltaSeconds;
    if (clip->duration > 0.0f) {
        if (looping) {
            state.currentTime = fmodf(state.currentTime, clip->duration);
        } else {
            state.currentTime = glm::min(state.currentTime, glm::max(0.0f, clip->duration - 1.0f));
        }
    }

    state.finalMatrices.assign(MAX_BONES, glm::mat4(1.0f));
    calculateBoneTransformsAtTime(clip->root, glm::mat4(1.0f), *clip, state.currentTime, state.finalMatrices, boneInfoMap, globalInverseTransform, true);
}

const AnimationClip* findClip(const std::unordered_map<std::string, AnimationClip>& clips, const std::string& name)
{
    auto it = clips.find(name);
    return it == clips.end() ? nullptr : &it->second;
}
