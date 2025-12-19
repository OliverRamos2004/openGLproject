#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "shaderprogram.h"
#include "stb_image.h"
#include "mesh.h"
#include <glm.hpp>
#include <gtc/matrix_transform.hpp>
#include <gtc/type_ptr.hpp>
#include <iostream>
#include "camera.h"
#include "light_config.h"


void framebuffer_size_callback(GLFWwindow *window, int width, int height);

void mouse_callback(GLFWwindow *window, double xpos, double ypos);

void scroll_callback(GLFWwindow *window, double xoffset, double yoffset);

void createShadowMap(GLuint &depthFBO, GLuint &depthMap, int SHADOW_WIDTH, int SHADOW_HEIGHT);

void createPointShadowMap(GLuint &depthFBO, GLuint &depthCubemap, int SHADOW_SIZE);

std::vector<glm::mat4> buildPointLightMatrices(glm::vec3 lightPos, float nearPlane, float farPlane);

float deltaTime = 0.0f; // Time between current frame and last frame
float lastFrame = 0.0f; // Time of last frame

int SCR_WIDTH = 800;
int SCR_HEIGHT = 600;
// NEW: shadow map size
GLuint depthFBO;
GLuint depthMap;
const unsigned int SHADOW_WIDTH = 2048, SHADOW_HEIGHT = 2048;
Camera camera;

// ============================================================
// POINT LIGHT SHADOW MAPPING GLOBALS
// ============================================================
const unsigned int POINT_SHADOW_SIZE = 1024; // cubemap resolution
GLuint pointDepthFBO = 0; // depth-only FBO
GLuint pointDepthCubemap = 0; // depth cubemap
float pointNear = 1.0f;
float pointFar = 25.0f;
// ===============================
// GALLERY PLACEMENT CONSTANTS
// ===============================
const float FLOOR_Y = 0.0f;

// These compensate for OBJ pivots (tune values)
float benchBaseYOffset = -0.35f;
float statueBaseYOffset = -1.5f;

const float BENCH_SCALE = 0.02f;
const float STATUE_SCALE = 0.01f;

unsigned int loadTexture(const std::string &path) {
    unsigned int textureID;
    glGenTextures(1, &textureID);

    int width, height, nrChannels;
    stbi_set_flip_vertically_on_load(true);
    unsigned char *data = stbi_load(path.c_str(), &width, &height, &nrChannels, 0);

    if (!data) {
        std::cout << "Failed to load texture: " << path << std::endl;
        return 0;
    }

    GLenum format = GL_RGB;
    if (nrChannels == 1) format = GL_RED;
    else if (nrChannels == 3) format = GL_RGB;
    else if (nrChannels == 4) format = GL_RGBA;

    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    stbi_image_free(data);
    return textureID;
}


int main() {
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW\n";
        return -1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    GLFWwindow *window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Hello CG", nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create GLFW window\n";
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetScrollCallback(window, scroll_callback);


    if (!gladLoadGLLoader((GLADloadproc) glfwGetProcAddress)) {
        std::cerr << "Failed to initialize GLAD\n";
        return -1;
    }
    //    createShadowMap(depthFBO, depthMap, SHADOW_WIDTH, SHADOW_HEIGHT);
    createPointShadowMap(pointDepthFBO, pointDepthCubemap, POINT_SHADOW_SIZE);


    std::vector<std::string> faces
    {
        std::string(ASSETS_DIR) + "skybox/right.jpg",
        std::string(ASSETS_DIR) + "skybox/left.jpg",
        std::string(ASSETS_DIR) + "skybox/top.jpg",
        std::string(ASSETS_DIR) + "skybox/bottom.jpg",
        std::string(ASSETS_DIR) + "skybox/front.jpg",
        std::string(ASSETS_DIR) + "skybox/back.jpg"
    };

    std::string cube_vertPath = std::string(SHADER_DIR) + "cube_vertex.vert";
    std::string cube_fragPath = std::string(SHADER_DIR) + "cube_fragment.frag";
    ShaderProgram cubeShader(cube_vertPath, cube_fragPath);
    Mesh cube(std::string(ASSETS_DIR) + "box.obj", cubeShader.getID());
    Mesh plane(std::string(ASSETS_DIR) + "plane.obj", cubeShader.getID());

    Mesh bench(std::string(ASSETS_DIR) + "bench.obj", cubeShader.getID());
    Mesh statue(std::string(ASSETS_DIR) + "12338_Statue_v1_L3.obj", cubeShader.getID());


    unsigned int loadTexture(const std::string &path);
    std::cout << "Loading p1...\n";
    unsigned int p1 = loadTexture(std::string(ASSETS_DIR) + "painting1.jpeg");
    std::cout << "p1 id = " << p1 << "\n";

    std::cout << "Loading p2...\n";
    unsigned int p2 = loadTexture(std::string(ASSETS_DIR) + "painting2.png");
    std::cout << "p1 id = " << p2 << "\n";

    std::cout << "Loading p3...\n";
    unsigned int p3 = loadTexture(std::string(ASSETS_DIR) + "painting3.png");
    std::cout << "p1 id = " << p3 << "\n";
    std::cout << "Loading p4...\n";
    unsigned int p4 = loadTexture(std::string(ASSETS_DIR) + "painting4.png");
    std::cout << "p1 id = " << p4 << "\n";
    std::cout << "Loading p5...\n";
    unsigned int p5 = loadTexture(std::string(ASSETS_DIR) + "painting5.png");
    std::cout << "p1 id = " << p5 << "\n";
    std::cout << "Loading p6...\n";
    unsigned int p6 = loadTexture(std::string(ASSETS_DIR) + "painting6.png");
    std::cout << "p1 id = " << p6 << "\n";
    std::cout << "Loading p7...\n";
    unsigned int p7 = loadTexture(std::string(ASSETS_DIR) + "painting7.png");
    std::cout << "p1 id = " << p7 << "\n";
    std::cout << "Loading p8...\n";
    unsigned int p8 = loadTexture(std::string(ASSETS_DIR) + "painting8.png");
    std::cout << "p1 id = " << p8 << "\n";

    auto clampTexture = [](unsigned int tex) {
        glBindTexture(GL_TEXTURE_2D, tex);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    };

    clampTexture(p1);
    clampTexture(p2);
    clampTexture(p3);
    clampTexture(p4);
    clampTexture(p5);
    clampTexture(p6);
    clampTexture(p7);
    clampTexture(p8);


    unsigned int paintVAO = 0, paintVBO = 0; {
        // A 1x1 quad centered at origin, facing +Z
        // layout: position(3), normal(3), texcoord(2)
        float quad[] = {
            //  x     y    z     nx ny nz    u   v
            -0.5f, -0.5f, 0.0f, 0, 0, 1, 0, 0,
            0.5f, -0.5f, 0.0f, 0, 0, 1, 1, 0,
            0.5f, 0.5f, 0.0f, 0, 0, 1, 1, 1,

            -0.5f, -0.5f, 0.0f, 0, 0, 1, 0, 0,
            0.5f, 0.5f, 0.0f, 0, 0, 1, 1, 1,
            -0.5f, 0.5f, 0.0f, 0, 0, 1, 0, 1
        };

        glGenVertexArrays(1, &paintVAO);
        glGenBuffers(1, &paintVBO);

        glBindVertexArray(paintVAO);
        glBindBuffer(GL_ARRAY_BUFFER, paintVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quad), quad, GL_STATIC_DRAW);

        // These attribute locations MUST match your vertex shader layout locations.
        // Most class setups are: 0=pos, 1=normal, 2=texcoords
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void *) 0);

        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void *) (3 * sizeof(float)));

        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void *) (6 * sizeof(float)));

        glBindVertexArray(0);
    }


    GLuint cube_diffuse = cubeShader.bindTexture2D("material.diffuse",
                                                   std::string(ASSETS_DIR) + "plastered_wall_diff_4k.png", 0,
                                                   false);
    GLuint cube_specular = cubeShader.bindTexture2D("material.specular",
                                                    std::string(ASSETS_DIR) + "plastered_wall_rough_4k.png", 1, false);
    GLuint plane_diffuse = cubeShader.bindTexture2D("material.diffuse", std::string(ASSETS_DIR) + "wood.png", 0, false);
    GLuint plane_specular = cubeShader.bindTexture2D("material.specular", std::string(ASSETS_DIR) + "wood_specular.png",
                                                     1, false);
    GLuint statue_diffuse = cubeShader.bindTexture2D("material.diffuse",
                                                     std::string(ASSETS_DIR) + "mossy_plasterwall.png", 0, false);
    GLuint statue_rough = cubeShader.bindTexture2D("material.specular",
                                                   std::string(ASSETS_DIR) + "mossy_plasterwall_rough.png",
                                                   1, false);
    GLuint skyboxID = cubeShader.bindCubeMap("skybox", faces, 2);
    cubeShader.setUniform("material.shininess", 32.0f);
    cubeShader.setUniform("material.alpha", 1.0f);

    std::string lighting_cube_vertPath = std::string(SHADER_DIR) + "cube_vertex.vert";
    std::string lighting_cube_fragPath = std::string(SHADER_DIR) + "lighting_cube_fragment.frag";
    ShaderProgram lightingCubeShader(lighting_cube_vertPath, lighting_cube_fragPath);
    Mesh lightingCube(std::string(ASSETS_DIR) + "box.obj", lightingCubeShader.getID());

    std::string skybox_vertPath = std::string(SHADER_DIR) + "skybox_vertex.vert";
    std::string skybox_fragPath = std::string(SHADER_DIR) + "skybox_fragment.frag";
    ShaderProgram skyboxShader(skybox_vertPath, skybox_fragPath);
    Mesh skybox(std::string(ASSETS_DIR) + "skybox.obj", skyboxShader.getID());
    skyboxShader.bindCubeMap("skybox", faces, /*textureUnit=*/0);

    std::string depth_vertPath = std::string(SHADER_DIR) + "depth_vertex.vert";
    std::string depth_fragPath = std::string(SHADER_DIR) + "depth_fragment.frag";
    ShaderProgram depthShader(depth_vertPath, depth_fragPath);

    std::string pointDepthVert = std::string(SHADER_DIR) + "point_depth_vertex.vert";
    std::string pointDepthFrag = std::string(SHADER_DIR) + "point_depth_fragment.frag";
    ShaderProgram pointDepthShader(pointDepthVert, pointDepthFrag);


    glm::vec3 cubePositions[] = {
        glm::vec3(0.0f, 0.0f, 0.0f),
        glm::vec3(2.0f, 5.0f, -15.0f),
        glm::vec3(-1.5f, -2.2f, -2.5f),
        glm::vec3(-3.8f, -2.0f, -12.3f),
        glm::vec3(2.4f, -0.4f, -3.5f),
        glm::vec3(-1.7f, 3.0f, -7.5f),
        glm::vec3(1.3f, -2.0f, -2.5f),
        glm::vec3(1.5f, 2.0f, -2.5f),
        glm::vec3(1.5f, 0.2f, -1.5f),
        glm::vec3(-1.3f, 1.0f, -1.5f)
    };

    gfx::SceneConfig cfg = gfx::makeDefaultSceneConfig();

    // ------------------------------------------------------------
    // Spotlights
    // 0 = statue spotlight (keep your existing one)
    // 1..N = painting spotlights
    // ------------------------------------------------------------
    // ------------------------------
    // Spotlights
    // 0 = statue spotlight
    // 1..7 = painting spotlights
    // ------------------------------

    // ============================================================
    // Six matrices for point light cubemap rendering


    // ============================================================

    // ============================================================
    // SPOTLIGHT SETUP (1 statue + 3 paintings)
    // ============================================================

    cfg.spotLights.clear();
    cfg.spotLights.reserve(4);

    // ------------------------------------------------------------
    // 0) Statue spotlight (center of room)
    // ------------------------------------------------------------
    cfg.spotLights.emplace_back();
    cfg.spotLights[0].position = glm::vec3(0.0f, 5.5f, 0.0f);
    cfg.spotLights[0].direction = glm::vec3(0.0f, -1.0f, 0.0f);

    cfg.spotLights[0].cutOff = glm::cos(glm::radians(10.0f));
    cfg.spotLights[0].outerCutOff = glm::cos(glm::radians(16.0f));

    cfg.spotLights[0].ambient = glm::vec3(0.0f);
    cfg.spotLights[0].diffuse = glm::vec3(0.30f, 0.28f, 0.25f);
    cfg.spotLights[0].specular = glm::vec3(0.12f);

    cfg.spotLights[0].constant = 1.0f;
    cfg.spotLights[0].linear = 0.14f;
    cfg.spotLights[0].quadratic = 0.07f;

    // ------------------------------------------------------------
    // Helper: add a painting spotlight (clamped to max = 4)
    // ------------------------------------------------------------
    auto addPaintingSpot = [&](glm::vec3 center, glm::vec3 wallNormal) {
        if ((int) cfg.spotLights.size() >= 4) return;

        cfg.spotLights.emplace_back();
        auto &s = cfg.spotLights.back();

        wallNormal = glm::normalize(wallNormal);

        float ceilingY = 5.5f;
        float forward = 1.25f;
        float downAim = 0.15f;

        // Position near ceiling, slightly into the room
        s.position = center + wallNormal * forward;
        s.position.y = ceilingY;

        // Aim at painting
        glm::vec3 target = center;
        target.y -= downAim;
        s.direction = glm::normalize(target - s.position);

        // Tight gallery cone
        s.cutOff = glm::cos(glm::radians(12.0f));
        s.outerCutOff = glm::cos(glm::radians(18.0f));

        // Subtle lighting (won't wash out walls)
        s.ambient = glm::vec3(0.0f);
        s.diffuse = glm::vec3(0.28f);
        s.specular = glm::vec3(0.10f);

        s.constant = 1.0f;
        s.linear = 0.14f;
        s.quadratic = 0.07f;
    };

    // ------------------------------------------------------------
    // 3 Painting spotlights (back wall)
    // ------------------------------------------------------------
    addPaintingSpot(glm::vec3(-3.0f, 1.5f, -6.0f), glm::vec3(0, 0, 1));
    addPaintingSpot(glm::vec3(0.0f, 1.5f, -6.0f), glm::vec3(0, 0, 1));
    addPaintingSpot(glm::vec3(3.0f, 1.5f, -6.0f), glm::vec3(0, 0, 1));

    // ------------------------------------------------------------
    // Send lights to shader

    // ------------------------------------------------------------
    gfx::applyDirLights(cubeShader, cfg.dirLights);
    gfx::applyPointLights(cubeShader, cfg.pointLights);
    gfx::applySpotLights(cubeShader, cfg.spotLights);

    auto pointShadowMatrices =
            buildPointLightMatrices(cfg.pointLights[0].position,
                                    pointNear, pointFar);


    while (!glfwWindowShouldClose(window)) {
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;
        camera.ProcessKeyboard(window, deltaTime);

        glEnable(GL_DEPTH_TEST);
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // ============================================================================
        // PASS 1B — POINT LIGHT SHADOW DEPTH PASS (6 renders into cubemap)
        // ============================================================================

        glViewport(0, 0, POINT_SHADOW_SIZE, POINT_SHADOW_SIZE);
        glBindFramebuffer(GL_FRAMEBUFFER, pointDepthFBO);

        pointDepthShader.use();
        pointDepthShader.setUniform("lightPos", cfg.pointLights[0].position);
        pointDepthShader.setUniform("farPlane", pointFar);

        for (int face = 0; face < 6; face++) {
            glFramebufferTexture2D(GL_FRAMEBUFFER,
                                   GL_DEPTH_ATTACHMENT,
                                   GL_TEXTURE_CUBE_MAP_POSITIVE_X + face,
                                   pointDepthCubemap,
                                   0);

            glClear(GL_DEPTH_BUFFER_BIT);

            pointDepthShader.setUniform("shadowMatrix", pointShadowMatrices[face]);

            // draw plane
            glm::mat4 model = glm::mat4(1.0f);
            pointDepthShader.setUniform("model", model);
            plane.draw();

            // draw statue (shadow caster)
            float pedestalTopY = 1.0f; // based on your pedestal cube

            model = glm::mat4(1.0f);
            model = glm::translate(
                model,
                glm::vec3(0.0f, pedestalTopY + statueBaseYOffset, 0.0f)
            );

            model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1, 0, 0));
            model = glm::scale(model, glm::vec3(0.01f)); // tune
            pointDepthShader.setUniform("model", model);
            statue.draw();


            // glm::mat4 m(1.0f);
            // m = glm::rotate(m, glm::radians(-90.0f), glm::vec3(1,0,0));
            // m = glm::scale(m, glm::vec3(0.01f));
            // cubeShader.setUniform("model", m);
            // bench.draw();


            // draw two benches (shadow casters)
            for (int b = 0; b < 2; b++) {
                model = glm::mat4(1.0f);
                model = glm::translate(
                    model,
                    glm::vec3(-2.0f + 4.0f * b, FLOOR_Y + benchBaseYOffset, 2.5f)
                );


                // stand-up fix
                model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1, 0, 0));

                // face toward statue (optional)
                model = glm::rotate(model, glm::radians(180.0f), glm::vec3(0, 1, 0));

                // scale fix
                model = glm::scale(model, glm::vec3(0.10f));

                pointDepthShader.setUniform("model", model);
                bench.draw();
            }
        }


        glBindFramebuffer(GL_FRAMEBUFFER, 0);


        glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        cubeShader.use();
        // ============================================================
        // PASS 2 — BIND POINT LIGHT SHADOWMAP
        // ============================================================
        glActiveTexture(GL_TEXTURE4);
        glBindTexture(GL_TEXTURE_CUBE_MAP, pointDepthCubemap);
        cubeShader.setUniform("pointShadowMap", 4);
        cubeShader.setUniform("farPlane", pointFar);


        cubeShader.use();
        glm::mat4 view = camera.GetViewMatrix();
        glm::mat4 projection = camera.GetProjection((float) SCR_WIDTH / SCR_HEIGHT);

        cubeShader.setUniform("view", view);
        cubeShader.setUniform("projection", projection);
        cubeShader.setUniform("viewPos", camera.Position);

        // Exposure for mood (only if your shader supports it)
        cubeShader.setUniform("exposure", 0.65f);
        cubeShader.setUniform("envMix", 0.0f); // default: walls should NOT reflect skybox


        // ---------- FLOOR ----------
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, plane_diffuse);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, plane_specular);

        glm::mat4 model = glm::mat4(1.0f);
        model = glm::scale(model, glm::vec3(12.0f, 1.0f, 12.0f)); // bigger floor
        cubeShader.setUniform("model", model);
        plane.draw();

        // ---------- WALLS (use cube mesh scaled thin) ----------
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, cube_diffuse);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, cube_specular);

        auto drawWall = [&](glm::vec3 pos, glm::vec3 scale) {
            glm::mat4 m(1.0f);
            m = glm::translate(m, pos);
            m = glm::scale(m, scale);
            cubeShader.setUniform("model", m);
            cube.draw();
        };

        // back wall, front wall, left wall, right wall
        float wallHeight = 10.0f;
        float wallY = FLOOR_Y + wallHeight * 0.3f; // bottom sits exactly on FLOOR_Y

        drawWall(glm::vec3(0.0f, wallY, -6.0f), glm::vec3(12.0f, wallHeight, 0.2f));
        drawWall(glm::vec3(0.0f, wallY, 6.0f), glm::vec3(12.0f, wallHeight, 0.2f));
        drawWall(glm::vec3(-6.0f, wallY, 0.0f), glm::vec3(0.2f, wallHeight, 12.0f));
        drawWall(glm::vec3(6.0f, wallY, 0.0f), glm::vec3(0.2f, wallHeight, 12.0f));

        enum WallFace { BACK, FRONT, LEFT, RIGHT };

        auto drawPainting = [&](WallFace face,
                                glm::vec3 center,
                                glm::vec2 size, // width, height
                                unsigned int texID) {
            if (texID == 0) return; // texture failed to load

            // Push off the wall to avoid z-fighting
            float eps = 0.12f;

            glm::mat4 m(1.0f);

            // Rotate quad to face correct direction
            // Assumes your quad's "front" faces +Z in object space
            if (face == BACK) {
                // back wall is at z = -something, quad should face +Z (no rotation)
                m = glm::translate(m, center + glm::vec3(0, 0, eps));
                // no rotate
            } else if (face == FRONT) {
                // front wall, quad should face -Z => rotate 180 around Y
                m = glm::translate(m, center + glm::vec3(0, 0, -eps));
                m = glm::rotate(m, glm::radians(180.0f), glm::vec3(0, 1, 0));
            } else if (face == RIGHT) {
                // right wall at +X, quad should face -X => rotate -90 around Y
                m = glm::translate(m, center + glm::vec3(-eps, 0, 0));
                m = glm::rotate(m, glm::radians(-90.0f), glm::vec3(0, 1, 0));
            } else if (face == LEFT) {
                // left wall at -X, quad should face +X => rotate +90 around Y
                m = glm::translate(m, center + glm::vec3(eps, 0, 0));
                m = glm::rotate(m, glm::radians(90.0f), glm::vec3(0, 1, 0));
            }

            // Scale quad to painting size (z scale doesn't matter for a quad)
            m = glm::scale(m, glm::vec3(size.x, size.y, 1.0f));

            cubeShader.use();
            cubeShader.setUniform("model", m);

            // make it unlit so it never goes black
            cubeShader.setUniform("unlit", true);

            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, texID);

            // If your shader expects material.specular too, bind something valid:
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, texID); // placeholder ok

            glBindVertexArray(paintVAO);
            glDrawArrays(GL_TRIANGLES, 0, 6);
            glBindVertexArray(0);

            cubeShader.setUniform("unlit", false);
        };


        float yTop = 3.3f;
        float yBot = 1.9f;


        // BACK wall (z = -6)
        drawPainting(BACK, glm::vec3(-3.0f, 1.5f, -6.0f), {2.0f, 1.3f}, p1);
        drawPainting(BACK, glm::vec3(0.0f, 1.5f, -6.0f), {2.0f, 1.3f}, p2);
        drawPainting(BACK, glm::vec3(3.0f, 1.5f, -6.0f), {2.0f, 1.3f}, p3);

        // RIGHT wall (x = +6)
        drawPainting(RIGHT, glm::vec3(6.0f, 1.5f, -2.5f), {2.0f, 1.3f}, p4);
        drawPainting(RIGHT, glm::vec3(6.0f, 1.5f, 2.5f), {2.0f, 1.3f}, p5);

        // LEFT wall (x = -6)
        drawPainting(LEFT, glm::vec3(-6.0f, 1.5f, 0.0f), {2.2f, 1.4f}, p6);

        // FRONT wall (z = +6)
        drawPainting(FRONT, glm::vec3(0.0f, 1.5f, 6.0f), {2.2f, 1.4f}, p7);


        // ---------- BENCHES ----------
        auto drawBench = [&](glm::vec3 pos, float yawDeg) {
            glm::mat4 m(1.0f);
            m = glm::translate(
                m,
                glm::vec3(pos.x, FLOOR_Y + benchBaseYOffset, pos.z)
            );


            // Stand-up fix (same idea that fixed the statue)
            m = glm::rotate(m, glm::radians(-90.0f), glm::vec3(1, 0, 0));

            // Face toward statue (optional)
            m = glm::rotate(m, glm::radians(yawDeg), glm::vec3(0, 1, 0));

            // Scale down (benches are usually huge in OBJ units)
            m = glm::scale(m, glm::vec3(0.10f));

            cubeShader.setUniform("model", m);
            bench.draw();
        };

        // place two benches
        drawBench(glm::vec3(-2.0f, 0.0f, 2.5f), 180.0f);
        drawBench(glm::vec3(2.0f, 0.0f, 2.5f), 180.0f);


        // ---------- PEDESTAL (simple cube) ----------
        glm::vec3 pedestalScale = glm::vec3(1.2f, 1.0f, 1.2f);
        glm::vec3 pedestalPos = glm::vec3(0.0f, FLOOR_Y + pedestalScale.y * 0.5f, 0.0f);

        // glm::mat4 pedestalModel(1.0f);
        // pedestalModel = glm::translate(pedestalModel, pedestalPos);
        // pedestalModel = glm::scale(pedestalModel, pedestalScale);
        // cubeShader.setUniform("model", pedestalModel);
        // cube.draw();

        float pedestalTopY = pedestalPos.y + pedestalScale.y * 0.5f;


        // ---------- STATUE ----------

        model = glm::mat4(1.0f);
        model = glm::translate(
            model,
            glm::vec3(0.0f, pedestalTopY + statueBaseYOffset, 0.0f)
        );

        model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1, 0, 0));
        model = glm::scale(model, glm::vec3(0.01f)); // start here: 0.01, then try 0.005 or 0.02
        cubeShader.setUniform("model", model);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, statue_diffuse); // temporary: wood; replace later with marble
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, statue_rough);

        statue.draw();


        // draw skybox as last
        glDepthFunc(GL_LEQUAL); // allow skybox to draw where depth == 1.0
        skyboxShader.use();
        glm::mat4 viewNoTrans = glm::mat4(glm::mat3(view));
        skyboxShader.setUniform("view", viewNoTrans);
        skyboxShader.setUniform("projection", projection);

        // reuse the same cube mesh as skybox geometry
        skybox.draw();
        glDepthFunc(GL_LESS); // restore default


        //
        // --- DEBUG LIGHT CUBE (optional) ---
        // If you don't want to see the cube at all, comment out this entire block.

        lightingCubeShader.use();
        lightingCubeShader.setUniform("view", view);
        lightingCubeShader.setUniform("projection", projection);

        glm::mat4 lightModel(1.0f);
        lightModel = glm::translate(lightModel, cfg.pointLights[0].position);
        lightModel = glm::scale(lightModel, glm::vec3(0.08f)); // small
        lightingCubeShader.setUniform("model", lightModel);

        lightingCube.draw();


        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}

void framebuffer_size_callback(GLFWwindow *window, int width, int height) {
    SCR_WIDTH = width;
    SCR_HEIGHT = height;
    glViewport(0, 0, width, height);
}


void mouse_callback(GLFWwindow *window, double xposIn, double yposIn) {
    float xpos = static_cast<float>(xposIn);
    float ypos = static_cast<float>(yposIn);

    if (camera.firstMouse) {
        camera.lastX = xpos;
        camera.lastY = ypos;
        camera.firstMouse = false;
    }

    float xoffset = xpos - camera.lastX;
    float yoffset = camera.lastY - ypos; // reversed since y-coordinates go from bottom to top

    camera.lastX = xpos;
    camera.lastY = ypos;

    camera.ProcessMouseMovement(xoffset, yoffset);
}

void scroll_callback(GLFWwindow *window, double xoffset, double yoffset) {
    camera.ProcessMouseScroll(static_cast<float>(yoffset));
}

void createShadowMap(GLuint &depthFBO, GLuint &depthMap,
                     int SHADOW_WIDTH, int SHADOW_HEIGHT) {
    glGenFramebuffers(1, &depthFBO);

    glGenTextures(1, &depthMap);
    glBindTexture(GL_TEXTURE_2D, depthMap);

    glTexImage2D(
        GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT,
        SHADOW_WIDTH, SHADOW_HEIGHT,
        0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr
    );

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

    float borderColor[] = {1.0f, 1.0f, 1.0f, 1.0f};
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

    glBindFramebuffer(GL_FRAMEBUFFER, depthFBO);
    glFramebufferTexture2D(
        GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
        GL_TEXTURE_2D, depthMap,
        0
    );

    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

// ---------------------------------------------------------------------------
// Create a depth cubemap for point light shadow mapping.
// This is analogous to your createShadowMap(), but for a cube texture.
// ---------------------------------------------------------------------------
void createPointShadowMap(GLuint &depthFBO, GLuint &depthCubemap,
                          int SHADOW_SIZE) {
    glGenFramebuffers(1, &depthFBO);

    glGenTextures(1, &depthCubemap);
    glBindTexture(GL_TEXTURE_CUBE_MAP, depthCubemap);

    for (int i = 0; i < 6; i++) {
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
                     0, GL_DEPTH_COMPONENT,
                     SHADOW_SIZE, SHADOW_SIZE,
                     0, GL_DEPTH_COMPONENT, GL_FLOAT,
                     nullptr);
    }

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    glBindFramebuffer(GL_FRAMEBUFFER, depthFBO);

    // DO NOT attach texture here — attach faces in the render loop
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}


// ---------------------------------------------------------------------------
// Build six view matrices for the cubemap.
// Orientation must match OpenGL cubemap coordinate system.
// ---------------------------------------------------------------------------
std::vector<glm::mat4> buildPointLightMatrices(glm::vec3 lightPos, float nearPlane, float farPlane) {
    glm::mat4 shadowProj = glm::perspective(glm::radians(90.0f),
                                            1.0f, nearPlane, farPlane);

    std::vector<glm::mat4> matrices;
    matrices.reserve(6);

    matrices.push_back(shadowProj *
                       glm::lookAt(lightPos, lightPos + glm::vec3(1, 0, 0), glm::vec3(0, -1, 0))); // +X
    matrices.push_back(shadowProj *
                       glm::lookAt(lightPos, lightPos + glm::vec3(-1, 0, 0), glm::vec3(0, -1, 0))); // -X
    matrices.push_back(shadowProj *
                       glm::lookAt(lightPos, lightPos + glm::vec3(0, 1, 0), glm::vec3(0, 0, 1))); // +Y
    matrices.push_back(shadowProj *
                       glm::lookAt(lightPos, lightPos + glm::vec3(0, -1, 0), glm::vec3(0, 0, -1))); // -Y
    matrices.push_back(shadowProj *
                       glm::lookAt(lightPos, lightPos + glm::vec3(0, 0, 1), glm::vec3(0, -1, 0))); // +Z
    matrices.push_back(shadowProj *
                       glm::lookAt(lightPos, lightPos + glm::vec3(0, 0, -1), glm::vec3(0, -1, 0))); // -Z

    return matrices;
}
