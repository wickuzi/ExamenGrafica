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
#include "../assimp/include/assimp/GltfMaterial.h"
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
#include <cctype>

// Para reproducción de video
#include <dshow.h>
#include <windows.h>
#include <string>
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

#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "oleaut32.lib")
#pragma comment(lib, "uuid.lib")
#pragma comment(lib, "strmiids.lib")
// Agregar al inicio del archivo, cerca de otras variables globales
double mouseX = 0, mouseY = 0;
bool mouseLeftPressed = false;
bool mouseLeftWasPressed = false;

// Constantes para la cinemática
const std::string CINEMATIC_VIDEO_PATH = "Resource Files\\video_sh.wmv"; // Cambia por tu video
const int CINEMATIC_WIDTH = 1920;
const int CINEMATIC_HEIGHT = 1080;

const int MAX_BONES = 100;
const int MAX_BONE_INFLUENCE = 4;

// Agregar con las otras variables globales
HWND glfwHWND = nullptr;

void framebuffer_size_callback(GLFWwindow *window, int width, int height);
void mouse_callback(GLFWwindow *window, double xpos, double ypos);
void scroll_callback(GLFWwindow *window, double xoffset, double yoffset);
void processInput(GLFWwindow *window);
void updateThirdPersonCamera();
void initAudio();
void startBackgroundMusic();
void playInteractionSound();
void updateFootstepAudio();
void shutdownAudio();
void closeCinematicPlayer();
bool audioCommand(const std::string &command);
unsigned int loadTextureFromJpeg(const wchar_t *path);
struct HudTexture;
HudTexture createTextTexture(const wchar_t *text, const wchar_t *fontName, float fontSize, int width, int height, const Gdiplus::Color &color);
unsigned int loadTextureFromFile(const std::string &path, const std::string &directory, std::map<std::string, unsigned int> &loaded, bool allowGlobalTextureSearch = true);
unsigned int getWhiteTexture();
unsigned int loadMaterialTexture(aiMaterial *material, const aiScene *scene, const std::string &directory, std::map<std::string, unsigned int> &loaded, bool allowGlobalTextureSearch = true);
std::vector<unsigned int> loadMaterialTextureFallbacks(const std::string &path, std::unordered_map<std::string, unsigned int> &byName);
struct BoneInfo
{
    int id = 0;
    glm::mat4 offset = glm::mat4(1.0f);
};
struct MeshData
{
    unsigned int VAO = 0, VBO = 0, EBO = 0;
    unsigned int indexCount = 0;
    unsigned int texture = 0;
    bool hasTexture = false;
    bool hasBones = false;
    glm::vec3 materialColor = glm::vec3(1.0f);
    float materialAlpha = 1.0f;
    bool useTextureAlpha = false;
    bool useWhiteChromaKey = false;
    bool isFoliage = false;
    std::vector<glm::vec3> positions;
    std::vector<unsigned int> indices;
    bool renderable = true;
    bool collider = false;
    bool walkArea = false;
    bool walkZone = false;
    glm::vec3 aabbMin = glm::vec3(FLT_MAX);
    glm::vec3 aabbMax = glm::vec3(-FLT_MAX);
};
struct BoneVertexData
{
    std::array<int, MAX_BONE_INFLUENCE> ids;
    std::array<float, MAX_BONE_INFLUENCE> weights;
};
struct KeyPosition
{
    glm::vec3 position;
    float timeStamp = 0.0f;
};
struct KeyRotation
{
    glm::quat orientation;
    float timeStamp = 0.0f;
};
struct KeyScale
{
    glm::vec3 scale;
    float timeStamp = 0.0f;
};
struct AnimChannel
{
    std::string name;
    std::vector<KeyPosition> positions;
    std::vector<KeyRotation> rotations;
    std::vector<KeyScale> scales;
};
struct AnimNode
{
    std::string name;
    glm::mat4 transform = glm::mat4(1.0f);
    std::vector<AnimNode> children;
};
struct AnimationClip
{
    std::string name;
    float duration = 0.0f;
    float ticksPerSecond = 25.0f;
    AnimNode root;
    std::unordered_map<std::string, AnimChannel> channels;
    bool valid = false;
};
struct AnimationState
{
    const AnimationClip *current = nullptr;
    float currentTime = 0.0f;
    std::vector<glm::mat4> finalMatrices;
    bool currentLooping = true;
};
struct WalkTriangle
{
    glm::vec3 a, b, c;
    float minX, maxX, minZ, maxZ;
    float area;
};
struct WalkArea
{
    glm::vec3 a;
    glm::vec3 b;
    glm::vec3 c;
    float minX;
    float maxX;
    float floorY;
    float minZ;
    float maxZ;
};
struct SavePoint
{
    glm::vec3 position;
    glm::vec3 normal;
};
struct HorrorLight
{
    glm::vec3 position;
    glm::vec3 color;
    float intensity = 1.0f;
    float radius = 12.0f;
    bool flicker = false;
    float phase = 0.0f;
};
struct CollisionBox
{
    glm::vec3 min;
    glm::vec3 max;
};
struct HudTexture
{
    unsigned int texture = 0;
    int width = 0;
    int height = 0;
};

struct MenuItem
{
    std::string text;
    HudTexture normalTexture;
    HudTexture selectedTexture;
    float x, y;
    bool isSelected;
};

std::vector<MenuItem> menuItems;
int selectedItemIndex = 0;
glm::mat4 aiToGlm(const aiMatrix4x4 &from);
bool findNodeWorldTransform(aiNode *node, const std::string &targetName, const glm::mat4 &parentTransform, glm::mat4 &outTransform);
bool loadNodeWorldTransform(const std::string &path, const std::string &nodeName, glm::mat4 &outTransform);
bool loadNodeWorldTransforms(const std::string &path, const std::string &namePrefix, std::vector<glm::mat4> &outTransforms);
void processNode(const aiScene *scene, aiNode *node, const glm::mat4 &parentTransform, const std::string &directory, std::vector<MeshData> &meshes, glm::vec3 &aabbMin, glm::vec3 &aabbMax, std::map<std::string, unsigned int> &loaded, std::unordered_map<std::string, BoneInfo> *boneInfoMap, int *boneCounter, bool insideLightPos = false);
MeshData processMesh(aiMesh *mesh, const aiScene *scene, const glm::mat4 &transform, const std::string &directory, glm::vec3 &aabbMin, glm::vec3 &aabbMax, std::map<std::string, unsigned int> &loaded, std::unordered_map<std::string, BoneInfo> *boneInfoMap, int *boneCounter, bool renderable = true, bool collider = false);
std::vector<MeshData> loadModel(const std::string &path, glm::vec3 &outAABBMin, glm::vec3 &outAABBMax, std::unordered_map<std::string, BoneInfo> *boneInfoMap = nullptr, int *boneCounter = nullptr, glm::mat4 *outGlobalInverse = nullptr);
AnimationClip loadAnimationClip(const std::string &path, const std::string &name);
void updateAnimation(AnimationState &state, const AnimationClip *clip, float deltaSeconds, const std::unordered_map<std::string, BoneInfo> &boneInfoMap, int boneCount, const glm::mat4 &globalInverseTransform, bool looping);
void calculateBoneTransforms(const AnimNode &node, const glm::mat4 &parentTransform, const AnimationClip &clip, std::vector<glm::mat4> &finalMatrices, const std::unordered_map<std::string, BoneInfo> &boneInfoMap);
glm::mat4 interpolateChannelTransform(const AnimChannel &channel, float animationTime);
const AnimationClip *findClip(const std::unordered_map<std::string, AnimationClip> &clips, const std::string &name);
bool isWalkAreaName(const std::string &name);
bool isWalkZoneName(const std::string &name);
bool isSpawnMarkerName(const std::string &name);
std::vector<WalkTriangle> buildWalkTriangles(const std::vector<MeshData> &meshes, const glm::mat4 &modelTransform);
std::vector<WalkArea> buildWalkAreas(const std::vector<MeshData> &meshes, const glm::mat4 &modelTransform);
bool canWalkHere(float x, float z);
bool findWalkAreaHeightAt(float x, float z, float &outY);
glm::vec3 findWalkAreaSpawnPoint();
glm::vec3 findSpawnPoint();
glm::vec3 findTownVisualSpawnPoint();
bool findGroundHeightAt(float x, float z, float maxStepUp, float &outY);
bool findAnyGroundHeightAt(float x, float z, float &outY);
float findGroundHeight(float x, float z, float fallbackY);
void updateWalkBoundary();
bool isBlockedByCollisionBoxes(const glm::vec3 &position);
glm::vec3 closestPointOnTriangle(const glm::vec3 &p, const glm::vec3 &a, const glm::vec3 &b, const glm::vec3 &c);
bool findWallAttachment(const std::vector<MeshData> &meshes, const glm::mat4 &modelTransform, const glm::vec3 &anchor, const glm::vec3 &preferredNormal, glm::vec3 &outPosition, glm::vec3 &outNormal);
glm::mat4 makeWallModel(const glm::vec3 &position, const glm::vec3 &normal, const glm::vec3 &scale);
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
glm::vec3 initialPlayerSpawn(0.0f, 0.0f, 0.0f);
float playerGroundY = 0.0f;
float playerYaw = 180.0f;
float initialPlayerYaw = 180.0f;
std::vector<WalkTriangle> walkTriangles;
std::vector<WalkArea> walkAreas;
std::vector<CollisionBox> collisionBoxes;
glm::vec2 walkBoundaryMin(0.0f);
glm::vec2 walkBoundaryMax(0.0f);
bool hasWalkBoundary = false;
bool jumpRequested = false;
bool spaceWasPressed = false;
bool eWasPressed = false;
bool saveMenuOpen = false;
bool turnAnimationActive = false;
bool playerIsMoving = false;
bool footstepsPlaying = false;
bool backgroundMusicPlaying = false;
float footstepTimer = 0.0f;
bool shotgunAvailable = false;
bool shotgunCollected = false;
glm::vec3 shotgunPosition(0.0f);

// timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

const glm::vec3 SAVE_POINT_POSITION = glm::vec3(4.5f, 1.75f, 1.5f);
const glm::vec3 SAVE_POINT_WALL_NORMAL = glm::vec3(1.0f, 0.0f, 0.0f);
glm::vec3 savePointPosition = SAVE_POINT_POSITION;
glm::vec3 savePointNormal = SAVE_POINT_WALL_NORMAL;
std::vector<SavePoint> savePoints;
std::vector<HorrorLight> horrorLights;
const float SAVE_POINT_INTERACT_RADIUS = 1.45f;
const float SHOTGUN_INTERACT_RADIUS = 1.35f;
// Shared clear/fog color prevents a seam at the distant city horizon.
const glm::vec3 FOG_COLOR(0.34f, 0.38f, 0.35f);
// Distance/fog tuning: fog must become opaque before props are culled.
const float renderDistance = 28.0f;
const float fogStart = 2.5f;
const float fogEnd = 22.0f;
const bool showLightCubes = false;

float distanceToTransformedAabb(const glm::vec3 &point, const MeshData &mesh, const glm::mat4 &model)
{
    // Map transforms are scale + translation, but transforming every corner
    // also keeps this valid if rotation is introduced later.
    glm::vec3 worldMin(FLT_MAX);
    glm::vec3 worldMax(-FLT_MAX);
    for (int x = 0; x < 2; ++x)
        for (int y = 0; y < 2; ++y)
            for (int z = 0; z < 2; ++z)
            {
                const glm::vec3 corner(
                    x ? mesh.aabbMax.x : mesh.aabbMin.x,
                    y ? mesh.aabbMax.y : mesh.aabbMin.y,
                    z ? mesh.aabbMax.z : mesh.aabbMin.z);
                const glm::vec3 world = glm::vec3(model * glm::vec4(corner, 1.0f));
                worldMin = glm::min(worldMin, world);
                worldMax = glm::max(worldMax, world);
            }
    const glm::vec3 closest = glm::clamp(point, worldMin, worldMax);
    return glm::length(point - closest);
}

// Upload helper kept separate from drawing so light registration can grow into
// a chunk system without touching the render loop.
void sendHorrorLightToShader(Shader &shader, int index, const HorrorLight &light, float timeSeconds)
{
    // Slow breathing plus two incommensurate harmonics creates an organic,
    // non-repeating flutter without abrupt nightclub-style flashes.
    const float breath = sinf(timeSeconds * 0.72f + light.phase) * 0.10f;
    const float unstable = light.flicker
        ? sinf(timeSeconds * 2.35f + light.phase * 1.7f) * 0.08f +
          sinf(timeSeconds * 6.13f + light.phase * 0.43f) * 0.035f
        : 0.0f;
    const float flicker = glm::clamp(0.92f + breath + unstable, 0.72f, 1.10f);
    const float dynamicRadius = light.radius *
        (0.94f + sinf(timeSeconds * 0.58f + light.phase * 0.81f) * 0.06f);
    const std::string base = "pointLights[" + std::to_string(index) + "].";
    shader.setVec3(base + "position", light.position);
    shader.setVec3(base + "color", light.color * light.intensity * flicker);
    shader.setFloat(base + "constant", 1.0f);
    shader.setFloat(base + "linear", 2.0f / dynamicRadius);
    shader.setFloat(base + "quadratic", 1.0f / (dynamicRadius * dynamicRadius));
}
const float JAMES_FBX_SKIN_SCALE = 0.01f;
const glm::vec3 JAMES_FBX_SKIN_OFFSET(0.0f, -0.90f, 0.0f);
std::vector<unsigned int> jamesFallbackTextures;
std::unordered_map<std::string, unsigned int> jamesFallbackTexturesByName;
bool allowSkinnedTextureSearch = false;
bool angelaCinematicPlaying = false;
int angelaConversationStage = 0; // 0=not met, 1/2=dialogue lines, 3=following
std::string angelaCinematicPath;
const float ANGELA_INTERACT_RADIUS = 1.65f;
const float ANGELA_GROUND_OFFSET = 1.83f;
bool hasAngelaNode = false;
glm::vec3 angelaPosition(0.0f);

// default orientation tweak (degrees) to make model face +Z upright; adjust if needed
const float MODEL_ROT_X = 0.0f;
const float MODEL_ROT_Y = 0.0f;
const float MODEL_ROT_Z = 0.0f; // try X=90, Y=180, Z=0 for upright facing +Z
// The render transform already rebases James to his feet. Keep only a tiny
// lift to avoid z-fighting with the authored walk-area surface.
const float JAMES_HEIGHT_OFFSET = 0.02f;
const float MAP_TARGET_SIZE = 38.0f;
const float TOWN_VISUAL_TARGET_SIZE = 180.0f;

// Función mejorada para crear texturas con contorno y sombra
HudTexture createStyledTextTexture(const wchar_t *text, const wchar_t *fontName, float fontSize, int width, int height,
                                   const Gdiplus::Color &color, const Gdiplus::Color &outlineColor = Gdiplus::Color(0, 0, 0, 200),
                                   bool hasShadow = true, const Gdiplus::Color &shadowColor = Gdiplus::Color(0, 0, 0, 180))
{
    HudTexture result{};
    Gdiplus::Bitmap bitmap(width, height, PixelFormat32bppARGB);
    Gdiplus::Graphics graphics(&bitmap);
    graphics.Clear(Gdiplus::Color(0, 0, 0, 0));
    graphics.SetTextRenderingHint(Gdiplus::TextRenderingHintAntiAliasGridFit);

    Gdiplus::FontFamily requestedFamily(fontName);
    const Gdiplus::FontFamily *family = requestedFamily.IsAvailable()
                                            ? &requestedFamily
                                            : Gdiplus::FontFamily::GenericSerif();
    Gdiplus::Font font(family, fontSize, Gdiplus::FontStyleRegular, Gdiplus::UnitPixel);

    Gdiplus::StringFormat format;
    format.SetAlignment(Gdiplus::StringAlignmentCenter);
    format.SetLineAlignment(Gdiplus::StringAlignmentCenter);
    Gdiplus::RectF layout(0.0f, 0.0f, static_cast<Gdiplus::REAL>(width), static_cast<Gdiplus::REAL>(height));

    // Dibujar sombra
    if (hasShadow)
    {
        Gdiplus::SolidBrush shadowBrush(shadowColor);
        Gdiplus::RectF shadowLayout(layout.X + 2.0f, layout.Y + 2.0f, layout.Width, layout.Height);
        graphics.DrawString(text, -1, &font, shadowLayout, &format, &shadowBrush);
    }

    // Dibujar contorno (dibujar el texto múltiples veces en diferentes direcciones)
    if (outlineColor.GetAlpha() > 0)
    {
        Gdiplus::SolidBrush outlineBrush(outlineColor);
        for (int dx = -1; dx <= 1; dx++)
        {
            for (int dy = -1; dy <= 1; dy++)
            {
                if (dx == 0 && dy == 0)
                    continue;
                Gdiplus::RectF outlineLayout(layout.X + dx, layout.Y + dy, layout.Width, layout.Height);
                graphics.DrawString(text, -1, &font, outlineLayout, &format, &outlineBrush);
            }
        }
    }

    // Dibujar texto principal
    Gdiplus::SolidBrush textBrush(color);
    graphics.DrawString(text, -1, &font, layout, &format, &textBrush);

    // Convertir a OpenGL texture
    Gdiplus::Rect rect(0, 0, width, height);
    Gdiplus::BitmapData bitmapData{};
    if (bitmap.LockBits(&rect, Gdiplus::ImageLockModeRead, PixelFormat32bppARGB, &bitmapData) != Gdiplus::Ok)
        return result;

    std::vector<unsigned char> pixels(width * height * 4);
    auto *srcBase = static_cast<unsigned char *>(bitmapData.Scan0);
    for (int y = 0; y < height; ++y)
    {
        const unsigned char *srcRow = srcBase + y * bitmapData.Stride;
        unsigned char *dstRow = pixels.data() + y * width * 4;
        for (int x = 0; x < width; ++x)
        {
            const unsigned char *src = srcRow + x * 4;
            unsigned char *dst = dstRow + x * 4;
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

unsigned int loadTextureFromJpeg(const wchar_t *path)
{
    Gdiplus::Bitmap bitmap(path);
    if (bitmap.GetLastStatus() != Gdiplus::Ok)
    {
        std::wstring w(path);
        std::string s(w.begin(), w.end());
        std::cout << "Failed to load texture: " << s << std::endl;
        return 0;
    }

    // bitmap.RotateFlip(Gdiplus::RotateNoneFlipY);

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
    auto *srcBase = static_cast<unsigned char *>(bitmapData.Scan0);
    for (UINT y = 0; y < height; ++y)
    {
        const unsigned char *srcRow = srcBase + y * bitmapData.Stride;
        unsigned char *dstRow = pixels.data() + y * width * 4;
        for (UINT x = 0; x < width; ++x)
        {
            const unsigned char *src = srcRow + x * 4;
            unsigned char *dst = dstRow + x * 4;
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

HudTexture createTextTexture(const wchar_t *text, const wchar_t *fontName, float fontSize, int width, int height, const Gdiplus::Color &color)
{
    HudTexture result{};
    Gdiplus::Bitmap bitmap(width, height, PixelFormat32bppARGB);
    Gdiplus::Graphics graphics(&bitmap);
    graphics.Clear(Gdiplus::Color(0, 0, 0, 0));
    graphics.SetTextRenderingHint(Gdiplus::TextRenderingHintAntiAliasGridFit);

    Gdiplus::FontFamily requestedFamily(fontName);
    const Gdiplus::FontFamily *family = requestedFamily.IsAvailable()
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
    auto *srcBase = static_cast<unsigned char *>(bitmapData.Scan0);
    for (int y = 0; y < height; ++y)
    {
        const unsigned char *srcRow = srcBase + y * bitmapData.Stride;
        unsigned char *dstRow = pixels.data() + y * width * 4;
        for (int x = 0; x < width; ++x)
        {
            const unsigned char *src = srcRow + x * 4;
            unsigned char *dst = dstRow + x * 4;
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
    CINEMATIC,
    PLAYING,
    PAUSED
};

// Variables globales para el video (reemplaza las anteriores)
IGraphBuilder *g_pGraph = nullptr;
IMediaControl *g_pControl = nullptr;
IMediaEvent *g_pEvent = nullptr;
IVideoWindow *g_pVideoWindow = nullptr;
bool g_bVideoPlaying = false;

GameState currentState = MENU;

void enterPlayingState()
{
    currentState = PLAYING;
    playerPosition = initialPlayerSpawn;
    playerYaw = initialPlayerYaw;
    thirdPersonYaw = 90.0f - initialPlayerYaw;
    firstMouse = true;
    playerGroundY = findGroundHeight(playerPosition.x, playerPosition.z, playerPosition.y);
    playerPosition.y = playerGroundY;
    updateThirdPersonCamera();
    startBackgroundMusic();
}

void finishAngelaCinematic()
{
    closeCinematicPlayer();
    angelaCinematicPlaying = false;
    currentState = PLAYING;
    angelaConversationStage = 1;
    spaceWasPressed = true;
    firstMouse = true;
}

// Inicializar y reproducir video
bool playCinematicVideo(const std::string &filepath, HWND parentWindow)
{
    // Limpiar anterior
    if (g_pControl)
    {
        g_pControl->Stop();
        g_pControl->Release();
        g_pControl = nullptr;
    }
    if (g_pEvent)
    {
        g_pEvent->Release();
        g_pEvent = nullptr;
    }
    if (g_pVideoWindow)
    {
        g_pVideoWindow->Release();
        g_pVideoWindow = nullptr;
    }
    if (g_pGraph)
    {
        g_pGraph->Release();
        g_pGraph = nullptr;
    }

    // Inicializar COM
    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

    // Crear FilterGraph
    HRESULT hr = CoCreateInstance(CLSID_FilterGraph, NULL, CLSCTX_INPROC_SERVER,
                                  IID_IGraphBuilder, (void **)&g_pGraph);
    if (FAILED(hr))
        return false;

    // Obtener interfaces
    g_pGraph->QueryInterface(IID_IMediaControl, (void **)&g_pControl);
    g_pGraph->QueryInterface(IID_IMediaEvent, (void **)&g_pEvent);
    g_pGraph->QueryInterface(IID_IVideoWindow, (void **)&g_pVideoWindow);

    if (!g_pControl || !g_pEvent || !g_pVideoWindow)
        return false;

    std::cout << "Video path: " << filepath << std::endl;
    std::cout << "Existe: " << std::filesystem::exists(filepath) << std::endl;
    // Cargar video
    std::wstring wpath(filepath.begin(), filepath.end());
    hr = g_pGraph->RenderFile(wpath.c_str(), NULL);
    if (FAILED(hr))
    {
        std::cout << "Error RenderFile: 0x"
                  << std::hex << hr << std::endl;
        return false;
    }

    // Configurar ventana
    g_pVideoWindow->put_Owner((OAHWND)parentWindow);
    g_pVideoWindow->put_WindowStyle(WS_CHILD | WS_CLIPSIBLINGS);
    RECT clientRect{};
    GetClientRect(parentWindow, &clientRect);
    g_pVideoWindow->SetWindowPosition(0, 0, clientRect.right - clientRect.left, clientRect.bottom - clientRect.top);
    g_pVideoWindow->put_Visible(OATRUE);

    // Reproducir
    g_pControl->Run();
    g_bVideoPlaying = true;

    return true;
}

// Verificar si el video terminó
bool isCinematicFinished()
{
    if (!g_pEvent || !g_bVideoPlaying)
        return true;

    long evCode = 0;
    LONG_PTR param1 = 0, param2 = 0;

    while (g_pEvent->GetEvent(&evCode, &param1, &param2, 0) == S_OK)
    {
        if (evCode == EC_COMPLETE)
        {
            g_pEvent->FreeEventParams(evCode, param1, param2);
            g_bVideoPlaying = false;
            return true;
        }
        g_pEvent->FreeEventParams(evCode, param1, param2);
    }

    return false;
}

// Detener y limpiar el video
void stopCinematicVideo()
{
    if (g_pControl)
    {
        g_pControl->Stop();
        g_bVideoPlaying = false;
    }
    if (g_pVideoWindow)
    {
        g_pVideoWindow->put_Visible(OAFALSE);
        g_pVideoWindow->put_Owner(0);
    }
}

// Cerrar reproductor
void closeCinematicPlayer()
{
    stopCinematicVideo();

    if (g_pVideoWindow)
    {
        g_pVideoWindow->Release();
        g_pVideoWindow = nullptr;
    }
    if (g_pControl)
    {
        g_pControl->Release();
        g_pControl = nullptr;
    }
    if (g_pEvent)
    {
        g_pEvent->Release();
        g_pEvent = nullptr;
    }
    if (g_pGraph)
    {
        g_pGraph->Release();
        g_pGraph = nullptr;
    }
    CoUninitialize();
}

void processMenuSelection(int index)
{
    if (index < 0 || index >= (int)menuItems.size())
        return;

    const std::string &selectedId = menuItems[index].text;
    if (selectedId == "newgame")
    {
        playInteractionSound();
        glfwHWND = GetActiveWindow();
        if (playCinematicVideo(CINEMATIC_VIDEO_PATH, glfwHWND))
        {
            currentState = CINEMATIC;
        }
        else
        {
            // Si falla, ir directo al juego
            enterPlayingState();
        }
    }
    else if (selectedId == "exit")
    {
        playInteractionSound();
        glfwSetWindowShouldClose(glfwGetCurrentContext(), true);
    }
}

void updateMenu(GLFWwindow *window, float deltaTime)
{
    // Detectar hover del mouse
    bool itemSelected = false;
    for (int i = 0; i < (int)menuItems.size(); i++)
    {
        float itemX = menuItems[i].x;
        float itemY = menuItems[i].y;
        float itemW = 400.0f;
        float itemH = 60.0f;

        if (mouseX >= itemX && mouseX <= itemX + itemW &&
            mouseY >= itemY && mouseY <= itemY + itemH)
        {
            selectedItemIndex = i;
            itemSelected = true;

            // Click con el mouse
            if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS && !mouseLeftWasPressed)
            {
                processMenuSelection(selectedItemIndex);
            }
            break;
        }
    }

    // Si no hay hover, mantener selección por teclado pero sin cambiar visualmente el hover
    if (!itemSelected)
    {
        // Navegación por teclado (WASD o flechas)
        if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        {
            static float keyCooldown = 0;
            if (keyCooldown <= 0)
            {
                selectedItemIndex = (selectedItemIndex - 1 + menuItems.size()) % menuItems.size();
                keyCooldown = 0.2f;
            }
            else
            {
                keyCooldown -= deltaTime;
            }
        }
        if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        {
            static float keyCooldown = 0;
            if (keyCooldown <= 0)
            {
                selectedItemIndex = (selectedItemIndex + 1) % menuItems.size();
                keyCooldown = 0.2f;
            }
            else
            {
                keyCooldown -= deltaTime;
            }
        }

        // Enter para seleccionar
        if (glfwGetKey(window, GLFW_KEY_ENTER) == GLFW_PRESS)
        {
            processMenuSelection(selectedItemIndex);
        }
    }

    mouseLeftWasPressed = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
}
int main()
{

    // Mostrar directorio de trabajo actual
    char cwd[MAX_PATH];
    GetCurrentDirectoryA(MAX_PATH, cwd);
    std::cout << "Current working directory: " << cwd << std::endl;

    // Listar archivos en el directorio actual
    std::cout << "Files in current directory:" << std::endl;
    for (const auto &entry : std::filesystem::directory_iterator("."))
    {
        std::cout << "  - " << entry.path().filename().string() << std::endl;
    }
    // glfw: initialize and configure
    // ------------------------------
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    // 4x MSAA keeps edges clean at a substantially lower fill-rate cost than 8x.
    glfwWindowHint(GLFW_SAMPLES, 4);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // glfw window creation
    // --------------------
    GLFWwindow *window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Silent Hill 2", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);
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
    try
    {
        resourceDir = std::filesystem::weakly_canonical(resourceDir);
    }
    catch (const std::filesystem::filesystem_error &e)
    {
        std::cout << "Resource path canonicalization failed: " << e.what() << std::endl;
        resourceDir = std::filesystem::absolute(resourceDir);
    }
    // build and compile our shader program using resourceDir
    // ------------------------------------
    Shader lightingShader((resourceDir / "2.2.basic_lighting.vs").string().c_str(), (resourceDir / "2.2.basic_lighting.fs").string().c_str());
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
        "skybox/Sky_AllSky_Overcast4_Low_Cam_2_Left+X.png",  // right (+X)
        "skybox/Sky_AllSky_Overcast4_Low_Cam_3_Right-X.png", // left (-X)

        "skybox/Sky_AllSky_Overcast4_Low_Cam_4_Up+Y.png",   // top (+Y)
        "skybox/Sky_AllSky_Overcast4_Low_Cam_5_Down-Y.png", // bottom (-Y)

        "skybox/Sky_AllSky_Overcast4_Low_Cam_0_Front+Z.png", // front
        "skybox/Sky_AllSky_Overcast4_Low_Cam_1_Back-Z.png"   // back
    };
    Skybox skybox(skyboxFaces);

    lightingShader.use();
    lightingShader.setInt("texture1", 0);

    // set basic material properties for Phong shading
    lightingShader.setFloat("material_shininess", 14.0f);
    lightingShader.setFloat("material_specularStrength", 0.18f);
    lightingShader.setFloat("material_ambientStrength", 0.25f);
    lightingShader.setVec3("fogColor", FOG_COLOR);
    lightingShader.setFloat("fogDensity", 0.105f);
    lightingShader.setFloat("fogStart", fogStart);
    lightingShader.setFloat("fogEnd", fogEnd);
    lightingShader.setInt("fogEnabled", 1);
    lightingShader.setFloat("objectAlpha", 1.0f);
    lightingShader.setFloat("alphaCutoff", 0.38f);
    lightingShader.setInt("useTextureAlpha", 1);
    lightingShader.setInt("useWhiteChromaKey", 0);
    lightingShader.setInt("foliageMaterial", 0);
    lightingShader.setFloat("emissiveStrength", 0.0f);

    unsigned int hudQuadVAO = 0;
    unsigned int hudQuadIndexCount = 0;
    createQuad(hudQuadVAO, hudQuadIndexCount);

    // richard's custom HUD textures
    HudTexture titleText;
    HudTexture startText;
    HudTexture exitText;
    HudTexture menuWallpaper;

    // Crear textos del menú con estilos mejorados
    titleText = createStyledTextTexture(
        L"                          SILENT HILL 2",
        L"Georgia", 72, 800, 100,
        Gdiplus::Color(220, 200, 180, 255), // Color dorado pálido
        Gdiplus::Color(40, 20, 15, 200),    // Contorno marrón oscuro
        true,                               // Con sombra
        Gdiplus::Color(0, 0, 0, 200)        // Sombra negra
    );

    // Opciones del menú
    struct MenuOption
    {
        std::string id;
        std::wstring text;
        float yOffset;
    };

    std::vector<MenuOption> options = {
        {"newgame", L"NUEVA PARTIDA", 380.0f},
        {"exit", L"SALIR", 470.0f}};

    for (const auto &opt : options)
    {
        MenuItem item;
        item.text = opt.id;
        item.x = (SCR_WIDTH - 400) / 2.0f;
        item.y = opt.yOffset;
        item.isSelected = false;

        // Texto normal (blanco con contorno)
        item.normalTexture = createStyledTextTexture(
            opt.text.c_str(), L"Georgia", 32, 400, 60,
            Gdiplus::Color(200, 200, 200, 255), // Gris claro
            Gdiplus::Color(30, 15, 10, 200),    // Contorno oscuro
            true,
            Gdiplus::Color(0, 0, 0, 180));

        // Texto seleccionado (rojo brillante con efecto de brillo)
        item.selectedTexture = createStyledTextTexture(
            opt.text.c_str(), L"Georgia", 36, 420, 65,
            Gdiplus::Color(255, 80, 60, 255), // Rojo intenso
            Gdiplus::Color(80, 20, 10, 220),  // Contorno rojo oscuro
            true,
            Gdiplus::Color(255, 40, 20, 100) // Sombra rojiza
        );

        menuItems.push_back(item);
    }

    std::filesystem::path wallpaperPath = resourceDir.parent_path() / "Resource Files" / "shwallpaper.jpeg";
    if (!std::filesystem::exists(wallpaperPath))
    {
        wallpaperPath = std::filesystem::path("Resource Files") / "shwallpaper.jpeg";
    }
    unsigned int wallTexID = loadTextureFromJpeg(wallpaperPath.wstring().c_str());

    // Si la carga fue exitosa, llenamos los datos de la estructura
    if (wallTexID != 0)
    {
        menuWallpaper.texture = wallTexID;
        menuWallpaper.width = SCR_WIDTH;   // Se estira al ancho de tu pantalla
        menuWallpaper.height = SCR_HEIGHT; // Se estira al alto de tu pantalla
        std::cout << "Wallpaper del menu cargado exitosamente con GDI+." << std::endl;
    }
    else
    {
        std::cout << "Error: No se pudo cargar el wallpaper del menu." << std::endl;
    }

    HudTexture saveTitleText = createTextTexture(L"SAVE GAME", L"Georgia", 42.0f, 400, 72, Gdiplus::Color(245, 245, 238, 228));
    HudTexture saveEmptyText = createTextTexture(L"EMPTY SLOT", L"Georgia", 24.0f, 320, 48, Gdiplus::Color(205, 222, 216, 210));
    HudTexture saveLocationText = createTextTexture(L"SILENT HILL ROAD", L"Georgia", 23.0f, 430, 64, Gdiplus::Color(220, 218, 210, 212));
    HudTexture saveTimeText = createTextTexture(L"19:42    2024/09/06    10:29", L"Georgia", 24.0f, 490, 48, Gdiplus::Color(210, 208, 200, 180));
    HudTexture savePromptText = createTextTexture(L"PRESS E TO RETURN", L"Georgia", 19.0f, 300, 40, Gdiplus::Color(230, 218, 210, 190));
    HudTexture saveInteractText = createTextTexture(L"PRESS E", L"Georgia", 20.0f, 112, 34, Gdiplus::Color(245, 230, 220, 218));
    HudTexture shotgunInteractText = createTextTexture(L"PRESS E TO GRAB", L"Georgia", 18.0f, 205, 34, Gdiplus::Color(245, 230, 220, 218));
    HudTexture angelaInteractText = createTextTexture(L"PRESS E TO INTERACT", L"Georgia", 18.0f, 245, 34, Gdiplus::Color(245, 230, 220, 218));
    HudTexture angelaDialogueText = createTextTexture(
        L"ANGELA: YO TAMBIEN ESTOY PERDIDA, HE VISTO A CRIATURAS EXTRANAS POR ESTOS LADOS, NO SE SI SEA SEGURO.   [ESPACIO]",
        L"Georgia", 20.0f, 1030, 72, Gdiplus::Color(245, 230, 220, 230));
    HudTexture jamesDialogueText = createTextTexture(
        L"JAMES: ESTA BIEN, SIGUEME Y SALDREMOS DE ACA.   [ESPACIO]",
        L"Georgia", 20.0f, 760, 54, Gdiplus::Color(245, 230, 220, 230));
    initAudio();

    // create title and menu textures by ropchard
    titleText =
        createTextTexture(
            L"SILENT HILL 2",
            L"Georgia",
            72,
            800,
            100,
            Gdiplus::Color(255, 255, 255, 255));

    startText =
        createTextTexture(
            L"PRESS ENTER TO START",
            L"Georgia",
            36,
            500,
            50,
            Gdiplus::Color(255, 220, 220, 220));

    exitText =
        createTextTexture(
            L"ESC TO EXIT",
            L"Georgia",
            30,
            300,
            50,
            Gdiplus::Color(255, 180, 180, 180));

    auto drawHudQuad = [&](unsigned int texture, float x, float y, float width, float height, const glm::vec3 &color, float alpha)
    {
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(x + width * 0.5f, y + height * 0.5f, 0.0f));
        model = glm::scale(model, glm::vec3(width, height, 1.0f));
        lightingShader.setMat4("model", model);
        lightingShader.setVec3("objectColor", color);
        lightingShader.setFloat("objectAlpha", alpha);
        lightingShader.setInt("useTextureAlpha", 1);
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
        resourceDir.parent_path().parent_path() / "models" / "james" / "james_sunderland.glb"};
    for (auto &p : jamesCandidates)
    {
        if (std::filesystem::exists(p))
        {
            jamesModelPath = p.string();
            std::cout << "Found James model at: " << jamesModelPath << std::endl;
            break;
        }
    }

    std::vector<std::filesystem::path> jamesTextureFallbackCandidates = {
        std::filesystem::path("models") / "james" / "james_sunderland.glb",
        std::filesystem::path("..") / "models" / "james" / "james_sunderland.glb",
        resourceDir.parent_path() / "models" / "james" / "james_sunderland.glb",
        resourceDir.parent_path().parent_path() / "models" / "james" / "james_sunderland.glb"};
    for (auto &p : jamesTextureFallbackCandidates)
    {
        if (std::filesystem::exists(p))
        {
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
    if (!jamesModelPath.empty())
    {
        jamesMeshes = loadModel(jamesModelPath, jamesAABBMin, jamesAABBMax, &jamesBoneInfo, &jamesBoneCount, &jamesGlobalInverseTransform);
        glm::vec3 jamesSize = jamesAABBMax - jamesAABBMin;
        float jamesHeight = glm::max(jamesSize.y, 0.001f);
        jamesRenderScale = 1.78f / jamesHeight;
        std::cout << "Loaded James meshes: " << jamesMeshes.size() << " bones=" << jamesBoneCount << std::endl;
        std::cout << "James size=(" << jamesSize.x << ", " << jamesSize.y << ", " << jamesSize.z << ") renderScale=" << jamesRenderScale << std::endl;
        for (size_t i = 0; i < jamesMeshes.size(); ++i)
        {
            std::cout << " James Mesh[" << i << "] indices=" << jamesMeshes[i].indexCount << " tex=" << jamesMeshes[i].texture << std::endl;
        }
    }

    std::unordered_map<std::string, AnimationClip> jamesAnimations;
    std::filesystem::path animDir = resourceDir.parent_path() / "models" / "jamesanimations";
    if (!std::filesystem::exists(animDir))
    {
        animDir = resourceDir.parent_path().parent_path() / "models" / "jamesanimations";
    }
    std::vector<std::pair<std::string, std::string>> animFiles = {
        {"idle", "idle.fbx"},
        {"walking", "walking.fbx"},
        {"strafe_left", "left strafe walking.fbx"},
        {"strafe_right", "right strafe walking.fbx"},
        {"turn_left", "left turn 90.fbx"},
        {"turn_right", "right turn 90.fbx"},
        {"jump", "jump.fbx"}};
    for (const auto &entry : animFiles)
    {
        std::filesystem::path p = animDir / entry.second;
        if (std::filesystem::exists(p))
        {
            AnimationClip clip = loadAnimationClip(p.string(), entry.first);
            if (clip.valid)
            {
                jamesAnimations[entry.first] = clip;
                std::cout << "Loaded animation: " << entry.first << " from " << p.string() << std::endl;
            }
        }
        else
        {
            std::cout << "Animation not found: " << p.string() << std::endl;
        }
    }
    std::filesystem::path gunAnimDir = resourceDir.parent_path() / "models" / "james" / "gunanimations";
    if (!std::filesystem::exists(gunAnimDir))
        gunAnimDir = resourceDir.parent_path().parent_path() / "models" / "james" / "gunanimations";
    const std::vector<std::pair<std::string, std::string>> gunAnimFiles = {
        {"rifle_idle", "Rifle Idle.fbx"},
        {"rifle_walk", "Rifle Walk.fbx"},
        {"rifle_run", "Rifle Run.fbx"},
        {"rifle_fire", "Firing Rifle.fbx"}};
    for (const auto &entry : gunAnimFiles)
    {
        const std::filesystem::path p = gunAnimDir / entry.second;
        if (!std::filesystem::exists(p))
        {
            std::cout << "Gun animation not found: " << p.string() << std::endl;
            continue;
        }
        AnimationClip clip = loadAnimationClip(p.string(), entry.first);
        if (clip.valid)
        {
            jamesAnimations[entry.first] = clip;
            std::cout << "Loaded gun animation: " << entry.first << std::endl;
        }
    }
    AnimationState jamesAnimState;
    jamesAnimState.finalMatrices.assign(MAX_BONES, glm::mat4(1.0f));
    jamesAnimState.current = findClip(jamesAnimations, "idle");

    // Load Angela and her authored idle animation.
    std::filesystem::path angelaDir = resourceDir.parent_path() / "models" / "angela";
    if (!std::filesystem::exists(angelaDir))
        angelaDir = resourceDir.parent_path().parent_path() / "models" / "angela";
    const std::filesystem::path angelaModelPath = angelaDir / "Angela.fbx";
    const std::filesystem::path angelaIdlePath = angelaDir / "animations" / "angela_idle.fbx";
    const std::filesystem::path angelaWalkPath = angelaDir / "animations" / "angela_walk.fbx";
    const std::filesystem::path angelaRunPath = angelaDir / "animations" / "Slow Run.fbx";
    angelaCinematicPath = std::filesystem::absolute(angelaDir / "video" / "jamesmeetsangela.wmv").string();
    glm::vec3 angelaAABBMin(FLT_MAX), angelaAABBMax(-FLT_MAX);
    std::vector<MeshData> angelaMeshes;
    std::unordered_map<std::string, BoneInfo> angelaBoneInfo;
    int angelaBoneCount = 0;
    glm::mat4 angelaGlobalInverseTransform(1.0f);
    float angelaRenderScale = 1.0f;
    AnimationClip angelaIdleClip;
    AnimationClip angelaWalkClip;
    AnimationClip angelaRunClip;
    AnimationState angelaAnimState;
    angelaAnimState.finalMatrices.assign(MAX_BONES, glm::mat4(1.0f));
    if (std::filesystem::exists(angelaModelPath))
    {
        // Angela's FBX stores absolute texture paths from the authoring PC;
        // permit basename lookup inside her local textures directory.
        allowSkinnedTextureSearch = true;
        // The standalone Angela.fbx is the texture/reference model but has no
        // skin hierarchy. The idle FBX contains that same mesh plus its rig.
        const std::filesystem::path &angelaSkinnedModelPath = std::filesystem::exists(angelaIdlePath)
            ? angelaIdlePath : angelaModelPath;
        angelaMeshes = loadModel(angelaSkinnedModelPath.string(), angelaAABBMin, angelaAABBMax,
                                 &angelaBoneInfo, &angelaBoneCount, &angelaGlobalInverseTransform);
        allowSkinnedTextureSearch = false;
        const float angelaHeight = glm::max(angelaAABBMax.y - angelaAABBMin.y, 0.001f);
        angelaRenderScale = 1.68f / angelaHeight;
        std::cout << "Loaded Angela meshes: " << angelaMeshes.size()
                  << " bones=" << angelaBoneCount << " scale=" << angelaRenderScale << std::endl;
    }
    if (std::filesystem::exists(angelaIdlePath))
    {
        angelaIdleClip = loadAnimationClip(angelaIdlePath.string(), "angela_idle");
        if (angelaIdleClip.valid)
            angelaAnimState.current = &angelaIdleClip;
    }
    if (std::filesystem::exists(angelaRunPath))
        angelaRunClip = loadAnimationClip(angelaRunPath.string(), "angela_slow_run");
    if (std::filesystem::exists(angelaWalkPath))
        angelaWalkClip = loadAnimationClip(angelaWalkPath.string(), "angela_walk");

    // Load map/scene if present. Supports either a clean models/map layout or the current SketchUp export under models/james.
    std::string mapModelPath;
    std::vector<std::filesystem::path> mapCandidates = {
        resourceDir.parent_path() / "models" / "town_visual.glb",
        std::filesystem::path("models") / "town_visual.glb",
        resourceDir.parent_path() / "models" / "map" / "town_visual.obj",
        std::filesystem::path("models") / "map" / "town_visual.obj",
        resourceDir.parent_path() / "models" / "map" / "town_visual.dae",
        std::filesystem::path("models") / "map" / "town_visual.dae",
        resourceDir.parent_path() / "models" / "town_visual.fbx",
        std::filesystem::path("models") / "town_visual.fbx",
        resourceDir.parent_path() / "models" / "town_visualseparated.glb",
        std::filesystem::path("models") / "town_visualseparated.glb",
        resourceDir.parent_path() / "models" / "town_visual.obj",
        std::filesystem::path("models") / "town_visual.obj",
        std::filesystem::path("models") / "map" / "model.dae",
        std::filesystem::path("models") / "map" / "scene.dae",
        std::filesystem::path("models") / "model.dae",
        std::filesystem::path("models") / "james" / "model.dae",
        resourceDir.parent_path() / "models" / "map" / "model.dae",
        resourceDir.parent_path() / "models" / "map" / "scene.dae",
        resourceDir.parent_path() / "models" / "model.dae",
        resourceDir.parent_path() / "models" / "james" / "model.dae"};
    for (auto &p : mapCandidates)
    {
        if (std::filesystem::exists(p))
        {
            std::error_code sizeError;
            uintmax_t fileSize = std::filesystem::file_size(p, sizeError);
            if (!sizeError && fileSize < 1024)
            {
                std::cout << "Skipping empty/suspicious map model: " << p.string()
                          << " size=" << fileSize << " bytes" << std::endl;
                continue;
            }
            mapModelPath = p.string();
            std::cout << "Found map model at: " << mapModelPath << std::endl;
            break;
        }
    }

    glm::vec3 mapAABBMin(FLT_MAX);
    glm::vec3 mapAABBMax(-FLT_MAX);
    std::vector<MeshData> mapMeshes;
    float mapScale = 1.0f;
    bool isTownVisualMap = false;
    bool hasSpawnNode = false;
    glm::mat4 spawnNodeTransform(1.0f);
    std::vector<glm::mat4> saveNodeTransforms;
    glm::mat4 shotgunNodeTransform(1.0f);
    bool hasShotgunNode = false;
    glm::mat4 angelaNodeTransform(1.0f);
    float angelaYaw = 0.0f;
    std::vector<glm::mat4> authoredWalkAreaTransforms;
    std::vector<glm::mat4> authoredLightPosTransforms;
    bool mapUsesZUp = false;
    glm::mat4 mapModelTransform = glm::mat4(1.0f);
    if (!mapModelPath.empty())
    {
        mapMeshes = loadModel(mapModelPath, mapAABBMin, mapAABBMax);
        int alphaMapMeshes = static_cast<int>(std::count_if(mapMeshes.begin(), mapMeshes.end(), [](const MeshData &mesh)
                                                            { return mesh.renderable && mesh.useTextureAlpha; }));
        std::cout << "Map alpha/cutout meshes: " << alphaMapMeshes << std::endl;
        glm::vec3 mapSize = mapAABBMax - mapAABBMin;
        std::string mapExt = std::filesystem::path(mapModelPath).extension().string();
        std::transform(mapExt.begin(), mapExt.end(), mapExt.begin(), ::tolower);
        mapUsesZUp = false;
        std::string mapStem = std::filesystem::path(mapModelPath).stem().string();
        std::transform(mapStem.begin(), mapStem.end(), mapStem.begin(), ::tolower);
        isTownVisualMap = mapStem.rfind("town_visual", 0) == 0;
        hasSpawnNode = loadNodeWorldTransform(mapModelPath, "spawn_player", spawnNodeTransform);
        loadNodeWorldTransforms(mapModelPath, "savepoint", saveNodeTransforms);
        hasShotgunNode = loadNodeWorldTransform(mapModelPath, "shotgunpos", shotgunNodeTransform);
        hasAngelaNode = loadNodeWorldTransform(mapModelPath, "angelainitialpos", angelaNodeTransform);
        loadNodeWorldTransforms(mapModelPath, "walkarea", authoredWalkAreaTransforms);
        loadNodeWorldTransforms(mapModelPath, "lightpos", authoredLightPosTransforms);
        if (authoredWalkAreaTransforms.size() != 12)
            std::cout << "WARNING: expected 12 authored walkareas, found "
                      << authoredWalkAreaTransforms.size() << std::endl;
        else
            std::cout << "Validated all 12 authored walkareas." << std::endl;
        float targetMapSize = isTownVisualMap ? TOWN_VISUAL_TARGET_SIZE : MAP_TARGET_SIZE;
        float mapHorizontalSize = glm::max(mapSize.x, mapSize.z);
        if (mapHorizontalSize > 0.001f)
        {
            mapScale = targetMapSize / mapHorizontalSize;
        }
        playerGroundY = 0.0f;
        playerPosition.y = playerGroundY;
        std::cout << "Loaded map meshes: " << mapMeshes.size()
                  << " size=(" << mapSize.x << ", " << mapSize.y << ", " << mapSize.z << ")"
                  << " scale=" << mapScale
                  << " targetSize=" << targetMapSize
                  << " externalAxisFix=false" << std::endl;
    }
    else
    {
        std::cout << "Map model path: NOT FOUND" << std::endl;
    }

    if (!mapMeshes.empty())
    {
        glm::vec3 mapCenter = (mapAABBMin + mapAABBMax) * 0.5f;
        mapModelTransform = glm::scale(mapModelTransform, glm::vec3(mapScale));
        mapModelTransform = glm::translate(mapModelTransform, glm::vec3(-mapCenter.x, -mapAABBMin.y, -mapCenter.z));
        walkTriangles = buildWalkTriangles(mapMeshes, mapModelTransform);
        walkAreas = buildWalkAreas(mapMeshes, mapModelTransform);
        updateWalkBoundary();

        // Navigation/helper volumes have served their purpose. Destroy their
        // GPU objects and remove them from the render collection entirely.
        // This makes drawing a walkarea/collision cube structurally impossible.
        for (MeshData &mesh : mapMeshes)
        {
            if (!mesh.renderable || mesh.walkArea || mesh.walkZone || mesh.collider)
            {
                if (mesh.VAO) glDeleteVertexArrays(1, &mesh.VAO);
                if (mesh.VBO) glDeleteBuffers(1, &mesh.VBO);
                if (mesh.EBO) glDeleteBuffers(1, &mesh.EBO);
                mesh.VAO = mesh.VBO = mesh.EBO = 0;
            }
        }
        mapMeshes.erase(std::remove_if(mapMeshes.begin(), mapMeshes.end(), [](const MeshData &mesh)
        {
            return !mesh.renderable || mesh.walkArea || mesh.walkZone || mesh.collider;
        }), mapMeshes.end());
        std::cout << "Renderable map meshes after helper purge: " << mapMeshes.size() << std::endl;

        if (hasShotgunNode)
        {
            shotgunPosition = glm::vec3((mapModelTransform * shotgunNodeTransform)[3]);
            shotgunAvailable = true;
            shotgunCollected = false;
            std::cout << "Shotgun pickup at (" << shotgunPosition.x << ", "
                      << shotgunPosition.y << ", " << shotgunPosition.z << ")" << std::endl;
        }

        if (hasAngelaNode)
        {
            const glm::mat4 angelaWorld = mapModelTransform * angelaNodeTransform;
            angelaPosition = glm::vec3(angelaWorld[3]);
            glm::vec3 angelaForward = glm::vec3(angelaWorld * glm::vec4(0.0f, 0.0f, 1.0f, 0.0f));
            if (glm::length(glm::vec2(angelaForward.x, angelaForward.z)) > 0.001f)
                angelaYaw = glm::degrees(atan2(angelaForward.x, angelaForward.z));
            float angelaGroundY = angelaPosition.y;
            if (findAnyGroundHeightAt(angelaPosition.x, angelaPosition.z, angelaGroundY))
                angelaPosition.y = angelaGroundY + ANGELA_GROUND_OFFSET;
            std::cout << "Angela initial position: (" << angelaPosition.x << ", "
                      << angelaPosition.y << ", " << angelaPosition.z << ") yaw=" << angelaYaw << std::endl;
        }

        collisionBoxes.clear();
        savePoints.clear();
        for (const glm::mat4 &nodeTransform : saveNodeTransforms)
        {
            glm::mat4 saveWorld = mapModelTransform * nodeTransform;
            glm::vec3 normal = glm::vec3(saveWorld * glm::vec4(0.0f, 0.0f, 1.0f, 0.0f));
            normal.y = 0.0f;
            normal = glm::length(normal) < 0.001f ? SAVE_POINT_WALL_NORMAL : glm::normalize(normal);
            savePoints.push_back({glm::vec3(saveWorld[3]), normal});
            std::cout << "SavePoint loaded at (" << saveWorld[3].x << ", " << saveWorld[3].y
                      << ", " << saveWorld[3].z << ")" << std::endl;
        }
        if (savePoints.empty())
            savePoints.push_back({SAVE_POINT_POSITION, SAVE_POINT_WALL_NORMAL});
        savePointPosition = savePoints.front().position;
        savePointNormal = savePoints.front().normal;

        // Blender empties named LightPos*, LIGHTPOS* or lightpos* become lights.
        // If the current export has none, edit the fallback positions below.
        horrorLights.clear();
        const std::array<glm::vec3, 3> horrorLightColors = {
            glm::vec3(0.35f, 0.55f, 0.35f), // dirty green
            glm::vec3(0.85f, 0.70f, 0.45f), // old yellow
            glm::vec3(0.75f, 0.72f, 0.60f)};// dirty white
        for (size_t i = 0; i < authoredLightPosTransforms.size(); ++i)
        {
            glm::vec3 position = glm::vec3((mapModelTransform * authoredLightPosTransforms[i])[3]);
            horrorLights.push_back({position, horrorLightColors[i % horrorLightColors.size()],
                                    1.65f, 15.0f, true, static_cast<float>(i) * 1.73f});
        }
        if (horrorLights.empty()) {
            // Temporary manual fallback: copy Blender world coordinates here.
            const std::array<glm::vec3, 3> manualLightPositions = {
                glm::vec3(4.5f, 3.2f, 1.5f), glm::vec3(-8.0f, 3.0f, -6.0f),
                glm::vec3(10.0f, 3.4f, -12.0f)};
            for (size_t i = 0; i < manualLightPositions.size(); ++i)
                horrorLights.push_back({manualLightPositions[i], horrorLightColors[i],
                                        1.55f, 14.0f, true, static_cast<float>(i) * 1.73f});
        }
        std::cout << "Invisible horror point lights: " << horrorLights.size() << std::endl;
        if (hasSpawnNode)
        {
            glm::vec3 spawnGlbPosition = glm::vec3(spawnNodeTransform[3]);
            glm::mat4 spawnWorld = mapModelTransform * spawnNodeTransform;
            initialPlayerSpawn = glm::vec3(spawnWorld[3]);
            glm::vec3 forward = glm::normalize(glm::vec3(spawnWorld * glm::vec4(0.0f, 0.0f, 1.0f, 0.0f)));
            if (glm::length(glm::vec2(forward.x, forward.z)) > 0.001f)
                initialPlayerYaw = glm::degrees(atan2(forward.x, forward.z));

            std::cout << "Spawn Blender/GLB position: "
                      << spawnGlbPosition.x << ", "
                      << spawnGlbPosition.y << ", "
                      << spawnGlbPosition.z << std::endl;
            std::cout << "Spawn after map transform: "
                      << initialPlayerSpawn.x << ", "
                      << initialPlayerSpawn.y << ", "
                      << initialPlayerSpawn.z << std::endl;
            std::cout << "Using spawn_player yaw=" << initialPlayerYaw << std::endl;
            if (!walkAreas.empty() && !canWalkHere(initialPlayerSpawn.x, initialPlayerSpawn.z))
            {
                std::cout << "Warning: spawn_player is outside every walkarea. James will still spawn there, but movement is restricted." << std::endl;
            }
        }
        else
        {
            initialPlayerSpawn = !walkAreas.empty() ? findWalkAreaSpawnPoint() : (isTownVisualMap ? findTownVisualSpawnPoint() : findSpawnPoint());
        }
        playerPosition = initialPlayerSpawn;
        playerYaw = initialPlayerYaw;
        thirdPersonYaw = 90.0f - initialPlayerYaw;
        if (hasSpawnNode)
        {
            const float jamesHeightOffset = JAMES_HEIGHT_OFFSET;
            float spawnSurfaceY = playerPosition.y;
            if (findWalkAreaHeightAt(playerPosition.x, playerPosition.z, spawnSurfaceY))
            {
                playerGroundY = spawnSurfaceY + jamesHeightOffset;
                std::cout << "Using walkarea height under spawn_player: "
                          << spawnSurfaceY << " jamesHeightOffset=" << jamesHeightOffset << std::endl;
            }
            else if (findAnyGroundHeightAt(playerPosition.x, playerPosition.z, spawnSurfaceY))
            {
                playerGroundY = spawnSurfaceY + jamesHeightOffset;
                std::cout << "Using highest map surface under spawn_player: "
                          << spawnSurfaceY << " jamesHeightOffset=" << jamesHeightOffset << std::endl;
            }
            else
            {
                playerGroundY = playerPosition.y + jamesHeightOffset;
                std::cout << "No map surface under spawn_player. Using spawn height plus jamesHeightOffset="
                          << jamesHeightOffset << std::endl;
            }
        }
        else
        {
            float snappedSpawnY = playerPosition.y;
            if (findAnyGroundHeightAt(playerPosition.x, playerPosition.z, snappedSpawnY))
            {
                playerGroundY = snappedSpawnY;
            }
            else
            {
                playerGroundY = playerPosition.y;
                std::cout << "Warning: no ground triangle found under spawn; using spawn Y." << std::endl;
            }
        }
        playerPosition.y = playerGroundY;
        std::cout << "James final OpenGL position: "
                  << playerPosition.x << ", "
                  << playerPosition.y << ", "
                  << playerPosition.z << std::endl;
        updateThirdPersonCamera();
        std::cout << "Walkable triangles: " << walkTriangles.size()
                  << " walkAreas=" << walkAreas.size()
                  << " collisionBoxes=" << collisionBoxes.size()
                  << " spawn=(" << playerPosition.x << ", " << playerPosition.y << ", " << playerPosition.z << ")"
                  << " initialGroundY=" << playerGroundY << std::endl;
    }

    glm::vec3 shotgunAABBMin(FLT_MAX), shotgunAABBMax(-FLT_MAX);
    std::vector<MeshData> shotgunMeshes;
    float shotgunRenderScale = 1.0f;
    const std::filesystem::path shotgunPath = gunAnimDir / "shotgun.glb";
    if (shotgunAvailable && std::filesystem::exists(shotgunPath))
    {
        shotgunMeshes = loadModel(shotgunPath.string(), shotgunAABBMin, shotgunAABBMax);
        const glm::vec3 shotgunSize = shotgunAABBMax - shotgunAABBMin;
        shotgunRenderScale = 1.15f / glm::max(glm::max(shotgunSize.x, shotgunSize.y), glm::max(shotgunSize.z, 0.001f));
        std::cout << "Loaded shotgun pickup meshes: " << shotgunMeshes.size() << std::endl;
    }
    else if (shotgunAvailable)
    {
        std::cout << "Shotgun model not found: " << shotgunPath.string() << std::endl;
        shotgunAvailable = false;
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
            // Actualizar lógica del menú
            updateMenu(window, deltaTime);

            // Limpiar buffers
            glClearColor(0.05f, 0.05f, 0.05f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            glDisable(GL_DEPTH_TEST);
            glDisable(GL_CULL_FACE);
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);

            // Configurar proyección Ortogonal
            glm::mat4 orthoProjection = glm::ortho(0.0f, (float)SCR_WIDTH, (float)SCR_HEIGHT, 0.0f, -1.0f, 1.0f);
            glm::mat4 viewIdentity = glm::mat4(1.0f);

            lightingShader.use();
            lightingShader.setMat4("projection", orthoProjection);
            lightingShader.setMat4("view", viewIdentity);
            lightingShader.setInt("fogEnabled", 0);
            lightingShader.setInt("useSkinning", 0);
            lightingShader.setFloat("objectAlpha", 1.0f);
            lightingShader.setInt("numPointLights", 0);
            lightingShader.setVec3("dirLight_direction", 0.0f, 0.0f, -1.0f);
            lightingShader.setVec3("dirLight_color", 1.0f, 1.0f, 1.0f);
            lightingShader.setFloat("material_ambientStrength", 1.0f);
            lightingShader.setFloat("material_specularStrength", 0.0f);

            // Dibujar wallpaper (con corrección de inversión)
            if (menuWallpaper.texture != 0)
            {
                drawHudQuad(menuWallpaper.texture, 0.0f, 0.0f,
                            (float)SCR_WIDTH, (float)SCR_HEIGHT,
                            glm::vec3(1.0f, 1.0f, 1.0f), 1.0f);
            }
            else
            {
                drawHudQuad(getWhiteTexture(), 0.0f, 0.0f,
                            (float)SCR_WIDTH, (float)SCR_HEIGHT,
                            glm::vec3(0.1f, 0.05f, 0.05f), 1.0f);
            }

            // Dibujar título
            // Al dibujar, centrar automáticamente
            float titleX = (SCR_WIDTH - titleText.width) / 2.0f;
            float titleY = 120.0f;
            drawHudQuad(titleText.texture, titleX, titleY,
                        (float)titleText.width, (float)titleText.height,
                        glm::vec3(1.0f), 1.0f);

            drawHudQuad(getWhiteTexture(), 0.0f, 0.0f,
                        (float)SCR_WIDTH, (float)SCR_HEIGHT,
                        glm::vec3(0.0f, 0.0f, 0.0f), 0.35f);

            // Dibujar opciones del menú
            for (int i = 0; i < (int)menuItems.size(); i++)
            {
                bool isSelected = (i == selectedItemIndex);
                HudTexture &tex = isSelected ? menuItems[i].selectedTexture : menuItems[i].normalTexture;

                // Efecto de animación para el seleccionado (pequeña escala)
                float scaleX = 1.0f, scaleY = 1.0f;
                if (isSelected)
                {
                    float pulse = 1.0f + sin(glfwGetTime() * 8.0f) * 0.03f;
                    scaleX = pulse;
                    scaleY = pulse;
                }

                drawHudQuad(tex.texture, menuItems[i].x, menuItems[i].y,
                            (float)tex.width, (float)tex.height,
                            glm::vec3(1.0f, 1.0f, 1.0f), 1.0f);

                // Después de dibujar el wallpaper, agregar una capa oscura semitransparente
            }
        }

        // En el bucle principal, después de processInput y antes del estado PLAYING
        else if (currentState == CINEMATIC)
        {
            // Verificar si el video terminó
            if (isCinematicFinished())
            {
                std::cout << "Cinemática finalizada, iniciando partida..." << std::endl;
                if (angelaCinematicPlaying)
                    finishAngelaCinematic();
                else
                {
                    closeCinematicPlayer();
                    enterPlayingState();
                }
            }

            // Permitir saltar la cinemática con ESC, ENTER o SPACE
            else if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS ||
                glfwGetKey(window, GLFW_KEY_ENTER) == GLFW_PRESS ||
                glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
            {

                std::cout << "Saltando cinemática..." << std::endl;
                if (angelaCinematicPlaying)
                    finishAngelaCinematic();
                else
                {
                    closeCinematicPlayer();
                    enterPlayingState();
                }
            }

            // Renderizar un fondo negro (el video se dibuja automáticamente)
            glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);

            Sleep(4);
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

            bool angelaShouldRun = false;
            bool angelaShouldWalk = false;
            if (hasAngelaNode && angelaConversationStage >= 3)
            {
                const float yawRadians = glm::radians(playerYaw);
                const glm::vec3 jamesForward(sinf(yawRadians), 0.0f, cosf(yawRadians));
                const glm::vec3 jamesRight(jamesForward.z, 0.0f, -jamesForward.x);
                const glm::vec3 followTarget = playerPosition - jamesForward * 1.15f + jamesRight * 0.38f;
                glm::vec3 toTarget = followTarget - angelaPosition;
                toTarget.y = 0.0f;
                const float followDistance = glm::length(toTarget);
                const bool jamesRunning = playerIsMoving && glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS;
                angelaShouldRun = jamesRunning;
                angelaShouldWalk = playerIsMoving && !jamesRunning;
                if (playerIsMoving && followDistance > 0.55f)
                {
                    const glm::vec3 direction = toTarget / followDistance;
                    const float followSpeed = jamesRunning ? 3.15f : 1.65f;
                    glm::vec3 nextAngelaPosition = angelaPosition + direction * glm::min(followDistance, followSpeed * deltaTime);
                    float followerGroundY = nextAngelaPosition.y - ANGELA_GROUND_OFFSET;
                    if (findWalkAreaHeightAt(nextAngelaPosition.x, nextAngelaPosition.z, followerGroundY) ||
                        findAnyGroundHeightAt(nextAngelaPosition.x, nextAngelaPosition.z, followerGroundY))
                    {
                        nextAngelaPosition.y = followerGroundY + ANGELA_GROUND_OFFSET;
                        angelaPosition = nextAngelaPosition;
                    }
                    angelaYaw = glm::degrees(atan2(direction.x, direction.z));
                }
            }

            // 1. Manejo del árbol de Animación de James
            const AnimationClip *desiredClip = findClip(jamesAnimations, "idle");
            bool desiredLooping = true;
            bool forwardPressed = glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS;
            bool backwardPressed = glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS;
            bool leftPressed = glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS;
            bool rightPressed = glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS;
            bool shiftPressed = glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS;
            bool firingPressed = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;

            if (shotgunCollected)
            {
                if (firingPressed && findClip(jamesAnimations, "rifle_fire"))
                    desiredClip = findClip(jamesAnimations, "rifle_fire");
                else if (playerIsMoving && shiftPressed && findClip(jamesAnimations, "rifle_run"))
                    desiredClip = findClip(jamesAnimations, "rifle_run");
                else if (playerIsMoving && findClip(jamesAnimations, "rifle_walk"))
                    desiredClip = findClip(jamesAnimations, "rifle_walk");
                else if (findClip(jamesAnimations, "rifle_idle"))
                    desiredClip = findClip(jamesAnimations, "rifle_idle");
            }
            else if (jumpRequested && findClip(jamesAnimations, "jump"))
            {
                desiredClip = findClip(jamesAnimations, "jump");
                desiredLooping = false;
            }
            else if ((forwardPressed || backwardPressed) && leftPressed && !rightPressed && findClip(jamesAnimations, "strafe_left"))
            {
                desiredClip = findClip(jamesAnimations, "strafe_left");
            }
            else if ((forwardPressed || backwardPressed) && rightPressed && !leftPressed && findClip(jamesAnimations, "strafe_right"))
            {
                desiredClip = findClip(jamesAnimations, "strafe_right");
            }
            else if ((forwardPressed || backwardPressed) && findClip(jamesAnimations, "walking"))
            {
                desiredClip = findClip(jamesAnimations, "walking");
            }
            else if ((leftPressed || turnAnimationActive) && !rightPressed && findClip(jamesAnimations, "turn_left"))
            {
                desiredClip = findClip(jamesAnimations, "turn_left");
                desiredLooping = false;
            }
            else if ((rightPressed || turnAnimationActive) && !leftPressed && findClip(jamesAnimations, "turn_right"))
            {
                desiredClip = findClip(jamesAnimations, "turn_right");
                desiredLooping = false;
            }

            updateAnimation(jamesAnimState, desiredClip, deltaTime, jamesBoneInfo, jamesBoneCount, jamesGlobalInverseTransform, desiredLooping);
            turnAnimationActive = desiredClip && !desiredLooping && desiredClip != findClip(jamesAnimations, "jump") && jamesAnimState.currentTime < desiredClip->duration - 1.0f;

            for (auto &matrix : jamesAnimState.finalMatrices)
            {
                matrix = glm::translate(glm::mat4(1.0f), JAMES_FBX_SKIN_OFFSET) *
                         glm::scale(glm::mat4(1.0f), glm::vec3(JAMES_FBX_SKIN_SCALE)) *
                         matrix;
            }
            const AnimationClip *desiredAngelaClip = &angelaIdleClip;
            if (angelaShouldRun && angelaRunClip.valid)
                desiredAngelaClip = &angelaRunClip;
            else if (angelaShouldWalk && angelaWalkClip.valid)
                desiredAngelaClip = &angelaWalkClip;
            if (hasAngelaNode && desiredAngelaClip->valid)
            {
                updateAnimation(angelaAnimState, desiredAngelaClip, deltaTime, angelaBoneInfo,
                                angelaBoneCount, angelaGlobalInverseTransform, true);
                // Angela and James are Mixamo FBX exports in centimeters. Use
                // the same skin-space conversion so both render at human scale.
                for (auto &matrix : angelaAnimState.finalMatrices)
                {
                    matrix = glm::translate(glm::mat4(1.0f), JAMES_FBX_SKIN_OFFSET) *
                             glm::scale(glm::mat4(1.0f), glm::vec3(JAMES_FBX_SKIN_SCALE)) *
                             matrix;
                }
            }
            jumpRequested = false;

            // Limpiar buffers para el frame 3D con el color de la niebla
            glClearColor(FOG_COLOR.r, FOG_COLOR.g, FOG_COLOR.b, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            // Transformaciones espaciales básicas 3D
            // Hard maximum visibility. Fog is already fully opaque at fogEnd,
            // so clipping at renderDistance cannot produce a visible pop.
            glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, renderDistance);
            glm::mat4 view = camera.GetViewMatrix();

            glEnable(GL_DEPTH_TEST);

            // Renderizar Skybox
            skyboxShader.use();
            skyboxShader.setFloat("fogTime", currentFrame);
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
            lightingShader.setVec3("fogCenter", playerPosition);
            lightingShader.setFloat("fogTime", currentFrame);
            lightingShader.setInt("useSkinning", 0);
            // Menu/HUD rendering uses full ambient light. Restore the darker
            // world material every frame so practical lights drive the scene.
            lightingShader.setFloat("material_shininess", 14.0f);
            lightingShader.setFloat("material_specularStrength", 0.18f);
            lightingShader.setFloat("material_ambientStrength", 0.25f);

            // Luz Direccional global del entorno
            lightingShader.setVec3("dirLight_direction", -0.2f, -1.0f, -0.3f);
            // Weak overcast fill: most contrast now comes from authored LightPos nodes.
            lightingShader.setVec3("dirLight_color", 0.18f, 0.20f, 0.17f);

            // Configuración de la luz puntal roja del punto de guardado
            // Distance-cull lights and select the nearest ones to bound shader cost.
            std::vector<std::pair<const HorrorLight *, glm::vec3>> activeHorrorLights;
            activeHorrorLights.reserve(horrorLights.size());
            for (const HorrorLight &light : horrorLights)
            {
                if (glm::length(light.position - camera.Position) <= renderDistance)
                    activeHorrorLights.push_back({&light, light.position});
            }
            std::sort(activeHorrorLights.begin(), activeHorrorLights.end(), [](const auto &a, const auto &b)
                      {
                          glm::vec3 deltaA = a.second - playerPosition;
                          glm::vec3 deltaB = b.second - playerPosition;
                          return glm::dot(deltaA, deltaA) < glm::dot(deltaB, deltaB);
                      });
            if (activeHorrorLights.size() > 8)
                activeHorrorLights.resize(8);

            // Invisible, dim fill light that follows above James. It has no VAO
            // and never submits geometry; only these point-light uniforms exist.
            HorrorLight jamesFollowLight{
                playerPosition + glm::vec3(0.0f, 2.05f, 0.0f),
                glm::vec3(0.70f, 0.69f, 0.61f),
                0.82f, 5.25f, true, 4.27f};
            HorrorLight angelaFollowLight{
                angelaPosition + glm::vec3(0.0f, 0.78f, 0.0f),
                glm::vec3(0.92f, 0.90f, 0.86f),
                0.68f, 4.8f, false, 1.19f};

            const bool shotgunLightActive = shotgunAvailable && !shotgunCollected;
            const bool angelaLightActive = hasAngelaNode && angelaConversationStage == 0;
            int numPoints = glm::min(static_cast<int>(savePoints.size() + activeHorrorLights.size() + 1 + (hasAngelaNode ? 1 : 0) +
                (shotgunLightActive ? 1 : 0) + (angelaLightActive ? 1 : 0)), 32);
            lightingShader.setInt("numPointLights", numPoints);
            int pointIndex = 0;
            for (size_t i = 0; i < savePoints.size() && pointIndex < numPoints; ++i, ++pointIndex)
            {
                glm::vec3 saveWorld = savePoints[i].position;
                glm::vec3 saveGlow = glm::vec3(1.0f, 0.035f, 0.018f) * 0.82f;
                std::string base = "pointLights[" + std::to_string(pointIndex) + "].";
                lightingShader.setVec3(base + "position", saveWorld);
                lightingShader.setVec3(base + "color", saveGlow);
                lightingShader.setFloat(base + "constant", 1.0f);
                lightingShader.setFloat(base + "linear", 0.14f);
                lightingShader.setFloat(base + "quadratic", 0.075f);
            }
            if (pointIndex < numPoints)
            {
                sendHorrorLightToShader(lightingShader, pointIndex, jamesFollowLight, currentFrame);
                ++pointIndex;
            }
            if (hasAngelaNode && pointIndex < numPoints)
            {
                sendHorrorLightToShader(lightingShader, pointIndex, angelaFollowLight, currentFrame);
                ++pointIndex;
            }
            if (shotgunLightActive && pointIndex < numPoints)
            {
                HorrorLight shotgunLight{
                    shotgunPosition + glm::vec3(0.0f, 0.85f, 0.0f),
                    glm::vec3(1.0f, 0.015f, 0.008f),
                    2.4f, 6.5f, true, 2.73f};
                sendHorrorLightToShader(lightingShader, pointIndex, shotgunLight, currentFrame);
                ++pointIndex;
            }
            if (angelaLightActive && pointIndex < numPoints)
            {
                HorrorLight angelaMeetLight{
                    angelaPosition + glm::vec3(0.0f, 0.45f, 0.0f),
                    glm::vec3(1.0f, 0.012f, 0.006f),
                    2.25f, 6.8f, true, 5.31f};
                sendHorrorLightToShader(lightingShader, pointIndex, angelaMeetLight, currentFrame);
                ++pointIndex;
            }
            for (const auto &activeLight : activeHorrorLights)
            {
                if (pointIndex >= numPoints)
                    break;
                const HorrorLight &light = *activeLight.first;
                sendHorrorLightToShader(lightingShader, pointIndex, light, currentFrame);
                ++pointIndex;
            }

            // 2. Dibujar el mapa / escenario
            if (!mapMeshes.empty())
            {
                glDisable(GL_CULL_FACE);
                lightingShader.setMat4("model", mapModelTransform);
                lightingShader.setFloat("alphaCutoff", 0.01f);
                for (auto &m : mapMeshes)
                {
                    // Walk zones are gameplay-only geometry and must never be visible.
                    if (!m.renderable || m.walkArea || m.walkZone)
                        continue;
                    // Cull each Blender sub-mesh against its nearest AABB point.
                    // Large streets/buildings stay while any part is nearby;
                    // completely distant objects submit no draw call at all.
                    if (distanceToTransformedAabb(camera.Position, m, mapModelTransform) > renderDistance)
                        continue;
                    // Alpha atlases in the city need a real cutout threshold;
                    // otherwise transparent atlas padding becomes white planes.
                    lightingShader.setFloat("alphaCutoff", m.isFoliage ? 0.46f : (m.useTextureAlpha ? 0.16f : 0.01f));
                    glm::vec3 renderColor = m.materialColor;
                    if (m.isFoliage)
                        renderColor *= glm::vec3(0.46f, 0.62f, 0.38f);
                    lightingShader.setVec3("objectColor", renderColor);
                    lightingShader.setFloat("objectAlpha", m.materialAlpha);
                    lightingShader.setInt("useTextureAlpha", m.useTextureAlpha ? 1 : 0);
                    lightingShader.setInt("useWhiteChromaKey", m.useWhiteChromaKey ? 1 : 0);
                    lightingShader.setInt("foliageMaterial", m.isFoliage ? 1 : 0);
                    glActiveTexture(GL_TEXTURE0);
                    glBindTexture(GL_TEXTURE_2D, m.texture);
                    glBindVertexArray(m.VAO);
                    glDrawElements(GL_TRIANGLES, (GLsizei)m.indexCount, GL_UNSIGNED_INT, 0);
                }
                glDisable(GL_CULL_FACE);
                lightingShader.setVec3("objectColor", 1.0f, 1.0f, 1.0f);
                lightingShader.setFloat("objectAlpha", 1.0f);
                lightingShader.setFloat("alphaCutoff", 0.38f);
                lightingShader.setInt("useTextureAlpha", 1);
                lightingShader.setInt("useWhiteChromaKey", 0);
                lightingShader.setInt("foliageMaterial", 0);
                lightingShader.setInt("useSkinning", 0);
            }

            // Shotgun pickup: authored position from the shotgunpos Blender empty.
            if (shotgunAvailable && !shotgunCollected && !shotgunMeshes.empty())
            {
                const glm::vec3 shotgunCenter = (shotgunAABBMin + shotgunAABBMax) * 0.5f;
                glm::mat4 shotgunModel(1.0f);
                shotgunModel = glm::translate(shotgunModel, shotgunPosition + glm::vec3(0.0f, 0.08f, 0.0f));
                shotgunModel = glm::rotate(shotgunModel, glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
                shotgunModel = glm::scale(shotgunModel, glm::vec3(shotgunRenderScale));
                shotgunModel = glm::translate(shotgunModel, -shotgunCenter);
                lightingShader.setMat4("model", shotgunModel);
                lightingShader.setInt("useSkinning", 0);
                lightingShader.setFloat("alphaCutoff", 0.01f);
                for (auto &m : shotgunMeshes)
                {
                    lightingShader.setVec3("objectColor", m.materialColor);
                    lightingShader.setFloat("objectAlpha", m.materialAlpha);
                    lightingShader.setInt("useTextureAlpha", m.useTextureAlpha ? 1 : 0);
                    glActiveTexture(GL_TEXTURE0);
                    glBindTexture(GL_TEXTURE_2D, m.texture);
                    glBindVertexArray(m.VAO);
                    glDrawElements(GL_TRIANGLES, (GLsizei)m.indexCount, GL_UNSIGNED_INT, 0);
                }
                lightingShader.setVec3("objectColor", 1.0f, 1.0f, 1.0f);
                lightingShader.setFloat("objectAlpha", 1.0f);
                lightingShader.setInt("useTextureAlpha", 1);
                lightingShader.setFloat("alphaCutoff", 0.38f);
            }

            // 3. Dibujar el modelo animado de James (Skinning habilitado)
            if (!jamesMeshes.empty())
            {
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

                for (int i = 0; i < glm::min(jamesBoneCount, MAX_BONES); ++i)
                {
                    lightingShader.setMat4("finalBonesMatrices[" + std::to_string(i) + "]", jamesAnimState.finalMatrices[i]);
                }
                for (auto &m : jamesMeshes)
                {
                    lightingShader.setVec3("objectColor", m.materialColor);
                    lightingShader.setFloat("objectAlpha", m.materialAlpha);
                    lightingShader.setInt("useTextureAlpha", 1);
                    glActiveTexture(GL_TEXTURE0);
                    glBindTexture(GL_TEXTURE_2D, m.texture);
                    glBindVertexArray(m.VAO);
                    glDrawElements(GL_TRIANGLES, (GLsizei)m.indexCount, GL_UNSIGNED_INT, 0);
                }
                lightingShader.setVec3("objectColor", 1.0f, 1.0f, 1.0f);
                lightingShader.setFloat("objectAlpha", 1.0f);
                lightingShader.setInt("useSkinning", 0);
            }

            // Angela at the authored angelainitialpos marker, idling in place.
            if (hasAngelaNode && !angelaMeshes.empty())
            {
                const glm::vec3 angelaCenter = (angelaAABBMin + angelaAABBMax) * 0.5f;
                glm::mat4 angelaModel(1.0f);
                angelaModel = glm::translate(angelaModel, angelaPosition);
                angelaModel = glm::rotate(angelaModel, glm::radians(angelaYaw), glm::vec3(0.0f, 1.0f, 0.0f));
                angelaModel = glm::scale(angelaModel, glm::vec3(angelaRenderScale));
                angelaModel = glm::translate(angelaModel, glm::vec3(-angelaCenter.x, -angelaAABBMin.y, -angelaCenter.z));
                lightingShader.setMat4("model", angelaModel);
                lightingShader.setInt("useSkinning", angelaBoneCount > 0 && angelaAnimState.current ? 1 : 0);
                for (int i = 0; i < glm::min(angelaBoneCount, MAX_BONES); ++i)
                    lightingShader.setMat4("finalBonesMatrices[" + std::to_string(i) + "]", angelaAnimState.finalMatrices[i]);
                for (size_t meshIndex = 0; meshIndex < angelaMeshes.size(); ++meshIndex)
                {
                    auto &m = angelaMeshes[meshIndex];
                    lightingShader.setVec3("objectColor", m.materialColor);
                    lightingShader.setFloat("objectAlpha", m.materialAlpha);
                    lightingShader.setInt("useTextureAlpha", 1);
                    lightingShader.setInt("useWhiteChromaKey", 0);
                    glActiveTexture(GL_TEXTURE0);
                    glBindTexture(GL_TEXTURE_2D, m.texture);
                    glBindVertexArray(m.VAO);
                    glDrawElements(GL_TRIANGLES, (GLsizei)m.indexCount, GL_UNSIGNED_INT, 0);
                }
                lightingShader.setVec3("objectColor", 1.0f, 1.0f, 1.0f);
                lightingShader.setFloat("objectAlpha", 1.0f);
                lightingShader.setInt("useSkinning", 0);
            }

            // Equipped shotgun. The rifle animations already pose both hands;
            // keep the prop aligned to James so it remains visible in every armed clip.
            if (shotgunCollected && !shotgunMeshes.empty())
            {
                const glm::vec3 shotgunCenter = (shotgunAABBMin + shotgunAABBMax) * 0.5f;
                glm::vec3 jamesCenter = (jamesAABBMin + jamesAABBMax) * 0.5f;
                glm::mat4 jamesWorld(1.0f);
                jamesWorld = glm::translate(jamesWorld, playerPosition);
                jamesWorld = glm::rotate(jamesWorld, glm::radians(MODEL_ROT_X), glm::vec3(1.0f, 0.0f, 0.0f));
                jamesWorld = glm::rotate(jamesWorld, glm::radians(MODEL_ROT_Y + playerYaw), glm::vec3(0.0f, 1.0f, 0.0f));
                jamesWorld = glm::rotate(jamesWorld, glm::radians(MODEL_ROT_Z), glm::vec3(0.0f, 0.0f, 1.0f));
                jamesWorld = glm::scale(jamesWorld, glm::vec3(jamesRenderScale));
                jamesWorld = glm::translate(jamesWorld, glm::vec3(-jamesCenter.x, -jamesAABBMin.y, -jamesCenter.z));

                auto animatedHandPosition = [&](const std::string &boneName, glm::vec3 &outPosition)
                {
                    auto bone = jamesBoneInfo.find(boneName);
                    if (bone == jamesBoneInfo.end() || bone->second.id < 0 || bone->second.id >= static_cast<int>(jamesAnimState.finalMatrices.size()))
                        return false;
                    // final = skinCorrection * globalInverse * animatedGlobal * offset.
                    // Removing the inverse-bind offset gives the animated bone origin.
                    glm::mat4 animatedBone = jamesAnimState.finalMatrices[bone->second.id] * glm::inverse(bone->second.offset);
                    outPosition = glm::vec3(jamesWorld * animatedBone * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));
                    return true;
                };

                glm::vec3 rightHand, leftHand;
                bool hasRightHand = animatedHandPosition("mixamorig:RightHand", rightHand);
                bool hasLeftHand = animatedHandPosition("mixamorig:LeftHand", leftHand);
                glm::mat4 equippedModel(1.0f);
                if (hasRightHand && hasLeftHand && glm::length(leftHand - rightHand) > 0.05f)
                {
                    // The shotgun's long axis is local X. Aim it from the trigger
                    // hand toward the support hand, so every clip drives the prop.
                    glm::vec3 supportAxis = glm::normalize(leftHand - rightHand);
                    // The barrel extends along local +X: point it toward the
                    // support hand while the stock remains by the trigger arm.
                    glm::vec3 modelX = supportAxis;
                    glm::vec3 modelZ = glm::cross(modelX, glm::vec3(0.0f, 1.0f, 0.0f));
                    if (glm::length(modelZ) < 0.01f)
                        modelZ = glm::vec3(0.0f, 0.0f, 1.0f);
                    else
                        modelZ = glm::normalize(modelZ);
                    glm::vec3 modelY = glm::normalize(glm::cross(modelZ, modelX));
                    glm::mat4 handBasis(1.0f);
                    handBasis[0] = glm::vec4(modelX, 0.0f);
                    handBasis[1] = glm::vec4(modelY, 0.0f);
                    handBasis[2] = glm::vec4(modelZ, 0.0f);
                    glm::vec3 gripCenter = rightHand + supportAxis * 0.24f - modelY * 0.035f;
                    equippedModel = glm::translate(glm::mat4(1.0f), gripCenter) * handBasis;
                }
                else
                {
                    // Safe fallback for models whose hand bones were renamed.
                    equippedModel = glm::translate(equippedModel, playerPosition);
                    equippedModel = glm::rotate(equippedModel, glm::radians(playerYaw), glm::vec3(0.0f, 1.0f, 0.0f));
                    equippedModel = glm::translate(equippedModel, glm::vec3(0.08f, 1.12f, 0.30f));
                    equippedModel = glm::rotate(equippedModel, glm::radians(258.0f), glm::vec3(0.0f, 1.0f, 0.0f));
                }
                equippedModel = glm::scale(equippedModel, glm::vec3(shotgunRenderScale));
                equippedModel = glm::translate(equippedModel, -shotgunCenter);

                lightingShader.setMat4("model", equippedModel);
                lightingShader.setInt("useSkinning", 0);
                lightingShader.setFloat("alphaCutoff", 0.01f);
                lightingShader.setFloat("emissiveStrength", 0.06f);
                for (auto &m : shotgunMeshes)
                {
                    lightingShader.setVec3("objectColor", m.materialColor);
                    lightingShader.setFloat("objectAlpha", m.materialAlpha);
                    lightingShader.setInt("useTextureAlpha", m.useTextureAlpha ? 1 : 0);
                    glActiveTexture(GL_TEXTURE0);
                    glBindTexture(GL_TEXTURE_2D, m.texture);
                    glBindVertexArray(m.VAO);
                    glDrawElements(GL_TRIANGLES, (GLsizei)m.indexCount, GL_UNSIGNED_INT, 0);
                }
                lightingShader.setVec3("objectColor", 1.0f, 1.0f, 1.0f);
                lightingShader.setFloat("objectAlpha", 1.0f);
                lightingShader.setInt("useTextureAlpha", 1);
                lightingShader.setFloat("alphaCutoff", 0.38f);
                lightingShader.setFloat("emissiveStrength", 0.0f);
            }

            // 4. Dibujar geometrías físicas del punto de guardado (Hojas de papel y discos)
            // Neon-red paper sheets authored by every savepoint* Blender node.
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, getWhiteTexture());
            lightingShader.setInt("useSkinning", 0);
            lightingShader.setInt("useTextureAlpha", 0);
            lightingShader.setInt("useWhiteChromaKey", 0);
            lightingShader.setInt("foliageMaterial", 0);
            lightingShader.setFloat("alphaCutoff", 0.01f);
            for (size_t i = 0; i < savePoints.size(); ++i)
            {
                const SavePoint &point = savePoints[i];
                if (glm::length(point.position - camera.Position) > renderDistance)
                    continue;
                float pulse = 0.92f + sinf(currentFrame * 2.15f + static_cast<float>(i) * 1.7f) * 0.08f;
                glm::mat4 paperModel = makeWallModel(
                    point.position + point.normal * 0.025f,
                    point.normal, glm::vec3(0.48f, 0.68f, 1.0f));
                lightingShader.setMat4("model", paperModel);
                lightingShader.setVec3("objectColor", 1.0f, 0.012f, 0.008f);
                lightingShader.setFloat("objectAlpha", 0.96f);
                lightingShader.setFloat("emissiveStrength", 1.35f * pulse);
                glBindVertexArray(hudQuadVAO);
                glDrawElements(GL_TRIANGLES, (GLsizei)hudQuadIndexCount, GL_UNSIGNED_INT, 0);
            }

            lightingShader.setVec3("objectColor", 1.0f, 1.0f, 1.0f);
            lightingShader.setFloat("objectAlpha", 1.0f);
            lightingShader.setInt("useTextureAlpha", 1);
            lightingShader.setFloat("alphaCutoff", 0.38f);
            lightingShader.setFloat("emissiveStrength", 0.0f);

            // 5. Gestión del Menú de Guardado superpuesto (UI / HUD)
            float saveDistance = FLT_MAX;
            for (const SavePoint &point : savePoints)
            {
                float distance = glm::length(glm::vec2(playerPosition.x - point.position.x, playerPosition.z - point.position.z));
                if (distance < saveDistance)
                {
                    saveDistance = distance;
                    savePointPosition = point.position;
                    savePointNormal = point.normal;
                }
            }
            bool savePointInRange = saveDistance <= SAVE_POINT_INTERACT_RADIUS;
            const bool shotgunInRange = shotgunAvailable && !shotgunCollected &&
                glm::length(glm::vec2(playerPosition.x - shotgunPosition.x, playerPosition.z - shotgunPosition.z)) <= SHOTGUN_INTERACT_RADIUS;
            const bool angelaInRange = hasAngelaNode && angelaConversationStage == 0 &&
                glm::length(glm::vec2(playerPosition.x - angelaPosition.x, playerPosition.z - angelaPosition.z)) <= ANGELA_INTERACT_RADIUS;
            const bool dialogueVisible = angelaConversationStage == 1 || angelaConversationStage == 2;

            if (saveMenuOpen || savePointInRange || shotgunInRange || angelaInRange || dialogueVisible)
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
                    if (!jamesMeshes.empty())
                    {
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

                        for (int i = 0; i < glm::min(jamesBoneCount, MAX_BONES); ++i)
                        {
                            lightingShader.setMat4("finalBonesMatrices[" + std::to_string(i) + "]", jamesAnimState.finalMatrices[i]);
                        }
                        for (auto &m : jamesMeshes)
                        {
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
                else if (dialogueVisible)
                {
                    const HudTexture &line = angelaConversationStage == 1 ? angelaDialogueText : jamesDialogueText;
                    const float panelWidth = glm::min(static_cast<float>(line.width + 36), 1120.0f);
                    drawHudQuad(getWhiteTexture(), 70.0f, 590.0f, panelWidth, 92.0f, glm::vec3(0.08f, 0.0f, 0.0f), 0.78f);
                    drawHudQuad(getWhiteTexture(), 70.0f, 590.0f, panelWidth, 2.0f, glm::vec3(0.70f, 0.08f, 0.06f), 0.55f);
                    drawHudQuad(line.texture, 88.0f, 602.0f, static_cast<float>(line.width), static_cast<float>(line.height), glm::vec3(1.0f), 1.0f);
                }
                else if (angelaInRange)
                {
                    drawHudQuad(getWhiteTexture(), 530.0f, 628.0f, 285.0f, 32.0f, glm::vec3(0.12f, 0.0f, 0.0f), 0.48f);
                    drawHudQuad(getWhiteTexture(), 530.0f, 628.0f, 285.0f, 1.0f, glm::vec3(0.62f, 0.08f, 0.06f), 0.42f);
                    drawHudQuad(angelaInteractText.texture, 548.0f, 630.0f, static_cast<float>(angelaInteractText.width), static_cast<float>(angelaInteractText.height), glm::vec3(1.0f), 1.0f);
                }
                else if (shotgunInRange)
                {
                    drawHudQuad(getWhiteTexture(), 560.0f, 628.0f, 235.0f, 32.0f, glm::vec3(0.12f, 0.0f, 0.0f), 0.48f);
                    drawHudQuad(getWhiteTexture(), 560.0f, 628.0f, 235.0f, 1.0f, glm::vec3(0.62f, 0.08f, 0.06f), 0.42f);
                    drawHudQuad(shotgunInteractText.texture, 575.0f, 630.0f, static_cast<float>(shotgunInteractText.width), static_cast<float>(shotgunInteractText.height), glm::vec3(1.0f), 1.0f);
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
                lightingShader.setFloat("material_ambientStrength", 0.25f);
                lightingShader.setFloat("material_specularStrength", 0.05f);
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
    for (auto &m : jamesMeshes)
    {
        if (m.VAO)
            glDeleteVertexArrays(1, &m.VAO);
        if (m.VBO)
            glDeleteBuffers(1, &m.VBO);
        if (m.EBO)
            glDeleteBuffers(1, &m.EBO);
        if (m.texture)
            glDeleteTextures(1, &m.texture);
    }
    for (auto &m : mapMeshes)
    {
        if (m.VAO)
            glDeleteVertexArrays(1, &m.VAO);
        if (m.VBO)
            glDeleteBuffers(1, &m.VBO);
        if (m.EBO)
            glDeleteBuffers(1, &m.EBO);
        if (m.texture)
            glDeleteTextures(1, &m.texture);
    }
    for (auto &m : shotgunMeshes)
    {
        if (m.VAO) glDeleteVertexArrays(1, &m.VAO);
        if (m.VBO) glDeleteBuffers(1, &m.VBO);
        if (m.EBO) glDeleteBuffers(1, &m.EBO);
        if (m.texture) glDeleteTextures(1, &m.texture);
    }
    for (auto &m : angelaMeshes)
    {
        if (m.VAO) glDeleteVertexArrays(1, &m.VAO);
        if (m.VBO) glDeleteBuffers(1, &m.VBO);
        if (m.EBO) glDeleteBuffers(1, &m.EBO);
        if (m.texture) glDeleteTextures(1, &m.texture);
    }
    shutdownAudio();
    Gdiplus::GdiplusShutdown(gdiplusToken);

    // Antes de glfwTerminate(), agregar:
    closeCinematicPlayer();
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
    if (currentState == MENU)
    {
        if (glfwGetKey(window, GLFW_KEY_ENTER) == GLFW_PRESS)
            processMenuSelection(selectedItemIndex);

        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
            glfwSetWindowShouldClose(window, true);

        return;
    }

    if (currentState == CINEMATIC)
        return;

    playerIsMoving = false;
    bool ePressed = glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS;
    bool escapePressed = glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS;
    if (saveMenuOpen)
    {
        if ((ePressed && !eWasPressed) || escapePressed)
        {
            if (ePressed && !eWasPressed)
                playInteractionSound();
            saveMenuOpen = false;
        }
        eWasPressed = ePressed;
        return;
    }

    if (escapePressed)
        glfwSetWindowShouldClose(window, true);

    bool dialogueSpacePressed = glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS;
    if (angelaConversationStage == 1 || angelaConversationStage == 2)
    {
        if (dialogueSpacePressed && !spaceWasPressed)
            ++angelaConversationStage;
        spaceWasPressed = dialogueSpacePressed;
        eWasPressed = ePressed;
        return;
    }

    const float angelaDistance = (hasAngelaNode && angelaConversationStage == 0)
        ? glm::length(glm::vec2(playerPosition.x - angelaPosition.x, playerPosition.z - angelaPosition.z))
        : FLT_MAX;
    if (ePressed && !eWasPressed && angelaDistance <= ANGELA_INTERACT_RADIUS)
    {
        playInteractionSound();
        glfwHWND = GetActiveWindow();
        if (playCinematicVideo(angelaCinematicPath, glfwHWND))
        {
            angelaCinematicPlaying = true;
            currentState = CINEMATIC;
        }
        else
        {
            angelaConversationStage = 1;
            spaceWasPressed = true;
        }
        eWasPressed = true;
        return;
    }

    const float shotgunDistance = (shotgunAvailable && !shotgunCollected)
        ? glm::length(glm::vec2(playerPosition.x - shotgunPosition.x, playerPosition.z - shotgunPosition.z))
        : FLT_MAX;
    if (ePressed && !eWasPressed && shotgunDistance <= SHOTGUN_INTERACT_RADIUS)
    {
        playInteractionSound();
        shotgunCollected = true;
        eWasPressed = true;
        return;
    }

    float saveDistance = FLT_MAX;
    for (const SavePoint &point : savePoints)
    {
        float distance = glm::length(glm::vec2(playerPosition.x - point.position.x, playerPosition.z - point.position.z));
        if (distance < saveDistance)
        {
            saveDistance = distance;
            savePointPosition = point.position;
            savePointNormal = point.normal;
        }
    }
    if (ePressed && !eWasPressed && saveDistance <= SAVE_POINT_INTERACT_RADIUS)
    {
        playInteractionSound();
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

    if (wPressed)
        movement += forward;
    if (sPressed)
        movement -= forward;
    if (aPressed && (wPressed || sPressed))
        movement -= right;
    if (dPressed && (wPressed || sPressed))
        movement += right;

    if (!wPressed && !sPressed)
    {
        float turnSpeed = 110.0f * deltaTime;
        if (aPressed && !dPressed)
            playerYaw -= turnSpeed;
        if (dPressed && !aPressed)
            playerYaw += turnSpeed;
    }

    if (glm::length(movement) > 0.001f)
    {
        movement = glm::normalize(movement);
        float speed = (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) ? 3.6f : 1.8f;
        glm::vec3 nextPosition = playerPosition + movement * speed * deltaTime;
        float nextWalkAreaY = playerPosition.y - JAMES_HEIGHT_OFFSET;
        if (findWalkAreaHeightAt(nextPosition.x, nextPosition.z, nextWalkAreaY))
        {
            nextPosition.y = nextWalkAreaY + JAMES_HEIGHT_OFFSET;
            if (!isBlockedByCollisionBoxes(nextPosition))
            {
                playerGroundY = nextPosition.y;
                playerPosition = nextPosition;
                playerYaw = glm::degrees(atan2(movement.x, movement.z));
                playerIsMoving = true;
            }
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
void framebuffer_size_callback(GLFWwindow *window, int width, int height)
{
    // make sure the viewport matches the new window dimensions; note that width and
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}

// glfw: whenever the mouse moves, this callback is called
// -------------------------------------------------------
void mouse_callback(GLFWwindow *window, double xposIn, double yposIn)
{
    // Guardar posición del mouse para el menú
    mouseX = xposIn;
    mouseY = yposIn;

    // Solo rotar cámara si estamos en PLAYING
    if (currentState == PLAYING)
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
        float yoffset = lastY - ypos;

        lastX = xpos;
        lastY = ypos;

        thirdPersonYaw += xoffset * 0.12f;
        thirdPersonPitch += yoffset * 0.08f;
        thirdPersonPitch = glm::clamp(thirdPersonPitch, -35.0f, 15.0f);
    }
}

// glfw: whenever the mouse scroll wheel scrolls, this callback is called
// ----------------------------------------------------------------------
void scroll_callback(GLFWwindow *window, double xoffset, double yoffset)
{
    thirdPersonDistance -= static_cast<float>(yoffset) * 0.35f;
    thirdPersonDistance = glm::clamp(thirdPersonDistance, 1.9f, 5.8f);
}

// Implementation modules: separated by responsibility while sharing the existing
// translation unit, preserving behavior and ownership during this refactor.
#include "systems/audio_system.inl"
#include "systems/texture_system.inl"
#include "systems/navigation_system.inl"
#include "systems/geometry_system.inl"
#include "systems/model_animation_system.inl"

