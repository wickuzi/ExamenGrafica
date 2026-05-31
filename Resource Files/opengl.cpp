#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

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
#include <filesystem>
#include <windows.h>
#include <gdiplus.h>
#include <cfloat>

// stb_image for TGA and other formats GDI+ may not support
#define STBI_NO_PKM
#define STB_IMAGE_IMPLEMENTATION
#include "../SOIL2/stb_image.h"

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow *window);
unsigned int loadTextureFromJpeg(const wchar_t* path);
unsigned int loadTextureFromFile(const std::string &path, const std::string &directory, std::map<std::string,unsigned int> &loaded);
unsigned int getWhiteTexture();
struct MeshData { unsigned int VAO=0, VBO=0, EBO=0; unsigned int indexCount=0; unsigned int texture=0; };
void processNode(const aiScene* scene, aiNode* node, const std::string &directory, std::vector<MeshData> &meshes, glm::vec3 &aabbMin, glm::vec3 &aabbMax, std::map<std::string,unsigned int> &loaded);
MeshData processMesh(aiMesh* mesh, const aiScene* scene, const std::string &directory, glm::vec3 &aabbMin, glm::vec3 &aabbMax, std::map<std::string,unsigned int> &loaded);
std::vector<MeshData> loadModel(const std::string &path, glm::vec3 &outAABBMin, glm::vec3 &outAABBMax);
// create a simple cube mesh for light source
void createCube(unsigned int &VAO, unsigned int &indexCount);

// settings
const unsigned int SCR_WIDTH = 1280;
const unsigned int SCR_HEIGHT = 720;

// camera
Camera camera(glm::vec3(0.0f, 0.0f, 3.0f));
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

// timing
float deltaTime = 0.0f;	
float lastFrame = 0.0f;

// lighting
glm::vec3 lightPos(1.5f, 1.5f, 1.0f);
float lightRadius = 2.0f;
glm::vec3 sceneCenter = glm::vec3(0.0f, 0.8f, 0.0f);
glm::vec3 whiteLocal = glm::vec3(0.8f, 2.0f, 0.8f);

// default orientation tweak (degrees) to make model face +Z upright; adjust if needed
const float MODEL_ROT_X = 90.0f;
const float MODEL_ROT_Y = 180.0f;
const float MODEL_ROT_Z = 0.0f; // try X=90, Y=180, Z=0 for upright facing +Z

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

    bitmap.RotateFlip(Gdiplus::RotateNoneFlipY);

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
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, static_cast<int>(width), static_cast<int>(height), 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
    glGenerateMipmap(GL_TEXTURE_2D);

    return texture;
}

int main()
{
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
    lightingShader.setFloat("material_specularStrength", 0.5f);
    lightingShader.setFloat("material_ambientStrength", 0.5f);

    // create a cube VAO to represent the point light
    unsigned int lightCubeVAO = 0;
    unsigned int lightCubeIndexCount = 0;
    createCube(lightCubeVAO, lightCubeIndexCount);
    // create a second (white) cube VAO to represent a bright white light near the model
    unsigned int whiteCubeVAO = 0;
    unsigned int whiteCubeIndexCount = 0;
    createCube(whiteCubeVAO, whiteCubeIndexCount);

    // Load James model
    std::string jamesModelPath;
    std::vector<std::filesystem::path> jamesCandidates = {
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

    std::cout << "=== Model Loading ===" << std::endl;
    std::cout << "James model path: " << (jamesModelPath.empty() ? "NOT FOUND" : jamesModelPath) << std::endl;

    // Load James model if found
    glm::vec3 jamesAABBMin(FLT_MAX);
    glm::vec3 jamesAABBMax(-FLT_MAX);
    std::vector<MeshData> jamesMeshes;
    if (!jamesModelPath.empty()) {
        jamesMeshes = loadModel(jamesModelPath, jamesAABBMin, jamesAABBMax);
        std::cout << "Loaded James meshes: " << jamesMeshes.size() << std::endl;
        for (size_t i = 0; i < jamesMeshes.size(); ++i) {
            std::cout << " James Mesh[" << i << "] indices=" << jamesMeshes[i].indexCount << " tex=" << jamesMeshes[i].texture << std::endl;
        }
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

        // calculate rotating red light position around James
        float angle = currentFrame * 1.5f; // rotation speed
        lightPos = sceneCenter + glm::vec3(
            glm::cos(angle) * lightRadius,
            1.5f,
            glm::sin(angle) * lightRadius
        );

        // pulsing "Silent Hill 2" style red vibration for the point light
        float pulse = 0.7f + 0.3f * sinf(currentFrame * 6.0f); // ranges ~[0.4,1.0]

        // input
        // -----
        processInput(window);

        // render
        // ------
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // view/projection transformations (needed for skybox)
        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
        glm::mat4 view = camera.GetViewMatrix();

        // render skybox first
        skybox.render(skyboxShader.ID, view, projection);

        // be sure to activate shader when setting uniforms/drawing objects
        lightingShader.use();
        lightingShader.setVec3("objectColor", 1.0f, 1.0f, 1.0f);
        lightingShader.setVec3("viewPos", camera.Position);

        // directional light
        lightingShader.setVec3("dirLight_direction", -0.2f, -1.0f, -0.3f);
        lightingShader.setVec3("dirLight_color", 1.0f, 1.0f, 1.0f);

        // two point lights: red rotating neon + bright white near the model
        int numPoints = 2;
        lightingShader.setInt("numPointLights", numPoints);
        {
            glm::vec3 ppos = lightPos;
            glm::vec3 pcolor = glm::vec3(1.0f, 0.5f, 0.5f) * pulse;
            std::string base = "pointLights[0].";
            lightingShader.setVec3(base + "position", ppos);
            lightingShader.setVec3(base + "color", pcolor);
            lightingShader.setFloat(base + "constant", 1.0f);
            lightingShader.setFloat(base + "linear", 0.05f);
            lightingShader.setFloat(base + "quadratic", 0.015f);
        }
        // second white point light
        {
            glm::vec3 whiteWorld = whiteLocal;
            glm::vec3 wcolor = glm::vec3(1.0f, 1.0f, 1.0f) * 0.15f;
            std::string base = "pointLights[1].";
            lightingShader.setVec3(base + "position", whiteWorld);
            lightingShader.setVec3(base + "color", wcolor);
            lightingShader.setFloat(base + "constant", 1.0f);
            lightingShader.setFloat(base + "linear", 0.02f);
            lightingShader.setFloat(base + "quadratic", 0.005f);
        }

        lightingShader.setMat4("projection", projection);
        lightingShader.setMat4("view", view);

        // Render James model
        if (!jamesMeshes.empty()) {
            glm::mat4 jamesModel = glm::mat4(1.0f);
            glm::vec3 jamesCenter = (jamesAABBMin + jamesAABBMax) * 0.5f;
            float jamesMinY = jamesAABBMin.y;
            jamesModel = glm::translate(jamesModel, glm::vec3(0.0f, -jamesMinY, 0.0f));
            jamesModel = glm::translate(jamesModel, glm::vec3(-jamesCenter.x, 0.0f, -jamesCenter.z));
            jamesModel = glm::rotate(jamesModel, glm::radians(MODEL_ROT_X), glm::vec3(1.0f, 0.0f, 0.0f));
            jamesModel = glm::rotate(jamesModel, glm::radians(MODEL_ROT_Y), glm::vec3(0.0f, 1.0f, 0.0f));
            jamesModel = glm::rotate(jamesModel, glm::radians(MODEL_ROT_Z), glm::vec3(0.0f, 0.0f, 1.0f));
            jamesModel = glm::scale(jamesModel, glm::vec3(0.8f));
            lightingShader.setMat4("model", jamesModel);
            for (auto &m : jamesMeshes) {
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, m.texture);
                glBindVertexArray(m.VAO);
                glDrawElements(GL_TRIANGLES, (GLsizei)m.indexCount, GL_UNSIGNED_INT, 0);
            }
        }

        // render the red rotating/pulsing light cube
        lightingShader.use();
        glm::mat4 lightModel = glm::mat4(1.0f);
        lightModel = glm::translate(lightModel, lightPos);
        lightModel = glm::scale(lightModel, glm::vec3(0.4f));
        lightingShader.setMat4("model", lightModel);
        lightingShader.setMat4("projection", projection);
        lightingShader.setMat4("view", view);
        lightingShader.setVec3("objectColor", 0.7f * pulse, 0.0f, 0.0f);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, getWhiteTexture());
        glBindVertexArray(lightCubeVAO);
        glDrawElements(GL_TRIANGLES, (GLsizei)lightCubeIndexCount, GL_UNSIGNED_INT, 0);

        // render a bright white cube (light source indicator)
        glm::mat4 whiteModel = glm::mat4(1.0f);
        whiteModel = glm::translate(whiteModel, whiteLocal);
        whiteModel = glm::scale(whiteModel, glm::vec3(0.35f));
        lightingShader.setMat4("model", whiteModel);
        lightingShader.setVec3("objectColor", 0.9f, 0.9f, 0.9f);
        glBindVertexArray(whiteCubeVAO);
        glDrawElements(GL_TRIANGLES, (GLsizei)whiteCubeIndexCount, GL_UNSIGNED_INT, 0);


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
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera.ProcessKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera.ProcessKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera.ProcessKeyboard(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera.ProcessKeyboard(RIGHT, deltaTime);

    // real-time controls for white light position (local model space)
    // J/L: left/right, I/K: up/down, U/O: forward/back
    glm::vec3 prevWhite = whiteLocal;
    float step = 0.05f;
    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) step *= 5.0f;
    if (glfwGetKey(window, GLFW_KEY_J) == GLFW_PRESS) whiteLocal.x -= step;
    if (glfwGetKey(window, GLFW_KEY_L) == GLFW_PRESS) whiteLocal.x += step;
    if (glfwGetKey(window, GLFW_KEY_I) == GLFW_PRESS) whiteLocal.y += step;
    if (glfwGetKey(window, GLFW_KEY_K) == GLFW_PRESS) whiteLocal.y -= step;
    if (glfwGetKey(window, GLFW_KEY_U) == GLFW_PRESS) whiteLocal.z += step;
    if (glfwGetKey(window, GLFW_KEY_O) == GLFW_PRESS) whiteLocal.z -= step;
    if (prevWhite != whiteLocal) {
        std::cout << "whiteLocal = (" << whiteLocal.x << ", " << whiteLocal.y << ", " << whiteLocal.z << ")\n";
    }
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

    camera.ProcessMouseMovement(xoffset, yoffset);
}

// glfw: whenever the mouse scroll wheel scrolls, this callback is called
// ----------------------------------------------------------------------
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    camera.ProcessMouseScroll(static_cast<float>(yoffset));
}


unsigned int loadTextureFromFile(const std::string &path, const std::string &directory, std::map<std::string,unsigned int> &loaded)
{
    std::string filename = path;
    // if path is relative, prepend directory
    if (filename.size() && filename[0] != '/' && filename.find(":\\") == std::string::npos && (filename.find("\\") != 0)) {
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
    std::wstring wfn(filename.begin(), filename.end());
    unsigned int tex = loadTextureFromJpeg(wfn.c_str());
    if (tex == 0) {
        // if GDI+ failed (commonly for .tga), try stb_image as a fallback
        std::string ext = filename.size() >= 4 ? filename.substr(filename.find_last_of('.') + 1) : std::string();
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
        if (ext == "tga" || ext == "ktx" || ext == "dds") {
            stbi_set_flip_vertically_on_load(1);
            int w=0,h=0,n=0;
            unsigned char* data = stbi_load(filename.c_str(), &w, &h, &n, 4);
            if (data) {
                glGenTextures(1, &tex);
                glBindTexture(GL_TEXTURE_2D, tex);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
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

MeshData processMesh(aiMesh* mesh, const aiScene* scene, const std::string &directory, glm::vec3 &aabbMin, glm::vec3 &aabbMax, std::map<std::string,unsigned int> &loaded)
{
    std::vector<float> vertices;
    std::vector<unsigned int> indices;
    // vertices: pos(3), normal(3), tex(2)
    for (unsigned int i = 0; i < mesh->mNumVertices; ++i)
    {
        // position
        vertices.push_back(mesh->mVertices[i].x);
        vertices.push_back(mesh->mVertices[i].y);
        vertices.push_back(mesh->mVertices[i].z);
        // update AABB
        aabbMin.x = std::min(aabbMin.x, mesh->mVertices[i].x);
        aabbMin.y = std::min(aabbMin.y, mesh->mVertices[i].y);
        aabbMin.z = std::min(aabbMin.z, mesh->mVertices[i].z);
        aabbMax.x = std::max(aabbMax.x, mesh->mVertices[i].x);
        aabbMax.y = std::max(aabbMax.y, mesh->mVertices[i].y);
        aabbMax.z = std::max(aabbMax.z, mesh->mVertices[i].z);
        // normal
        if (mesh->HasNormals()) {
            vertices.push_back(mesh->mNormals[i].x);
            vertices.push_back(mesh->mNormals[i].y);
            vertices.push_back(mesh->mNormals[i].z);
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
    }
    for (unsigned int i = 0; i < mesh->mNumFaces; ++i)
    {
        aiFace face = mesh->mFaces[i];
        for (unsigned int j = 0; j < face.mNumIndices; ++j)
            indices.push_back(face.mIndices[j]);
    }

    MeshData mdata;
    mdata.indexCount = (unsigned int)indices.size();

    glGenVertexArrays(1, &mdata.VAO);
    glGenBuffers(1, &mdata.VBO);
    glGenBuffers(1, &mdata.EBO);

    glBindVertexArray(mdata.VAO);
    glBindBuffer(GL_ARRAY_BUFFER, mdata.VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mdata.EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

    // pos
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // normal
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    // tex
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    glBindVertexArray(0);

    // load material textures (diffuse), including embedded textures in glTF/.glb
    unsigned int texid = 0;
    if (mesh->mMaterialIndex >= 0) {
        aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
        if (material->GetTextureCount(aiTextureType_DIFFUSE) > 0) {
            aiString str;
            material->GetTexture(aiTextureType_DIFFUSE, 0, &str);
            std::string texPath = str.C_Str();
            if (!texPath.empty() && texPath[0] == '*') {
                // embedded texture reference like "*0"
                int texIndex = atoi(texPath.c_str() + 1);
                if (texIndex >= 0 && scene->mNumTextures > (unsigned)texIndex) {
                    aiTexture* atex = scene->mTextures[texIndex];
                    unsigned int glt = 0;
                    if (atex->mHeight == 0) {
                        // compressed image in memory (e.g., PNG/JPEG)
                        int w=0,h=0,n=0;
                        unsigned char* img = stbi_load_from_memory((unsigned char*)atex->pcData, (int)atex->mWidth, &w, &h, &n, 4);
                        if (img) {
                            glGenTextures(1, &glt);
                            glBindTexture(GL_TEXTURE_2D, glt);
                            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
                            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
                            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
                            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, img);
                            glGenerateMipmap(GL_TEXTURE_2D);
                            stbi_image_free(img);
                        }
                    } else {
                        // uncompressed RGBA data in aiTexture->pcData as aiTexel
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
                        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
                        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
                        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
                        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, img.data());
                        glGenerateMipmap(GL_TEXTURE_2D);
                    }
                    texid = glt;
                }
            } else {
                texid = loadTextureFromFile(texPath, directory, loaded);
            }
        }
    }
    mdata.texture = texid;
    if (mdata.texture == 0) mdata.texture = getWhiteTexture();
    return mdata;
}

void processNode(const aiScene* scene, aiNode* node, const std::string &directory, std::vector<MeshData> &meshes, glm::vec3 &aabbMin, glm::vec3 &aabbMax, std::map<std::string,unsigned int> &loaded)
{
    for (unsigned int i = 0; i < node->mNumMeshes; ++i)
    {
        aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
        meshes.push_back(processMesh(mesh, scene, directory, aabbMin, aabbMax, loaded));
    }
    for (unsigned int i = 0; i < node->mNumChildren; ++i)
        processNode(scene, node->mChildren[i], directory, meshes, aabbMin, aabbMax, loaded);
}

std::vector<MeshData> loadModel(const std::string &path, glm::vec3 &outAABBMin, glm::vec3 &outAABBMax)
{
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_FlipUVs);
    std::vector<MeshData> meshes;
    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
        std::cerr << "ERROR::ASSIMP::" << importer.GetErrorString() << std::endl;
        return meshes;
    }
    // get directory
    std::string directory = path;
    size_t pos = directory.find_last_of("/\\");
    if (pos != std::string::npos) directory = directory.substr(0, pos);
    else directory = ".";
    std::map<std::string,unsigned int> loaded;
    processNode(scene, scene->mRootNode, directory, meshes, outAABBMin, outAABBMax, loaded);
    return meshes;
}
