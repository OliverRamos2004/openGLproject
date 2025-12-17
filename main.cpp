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

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void createShadowMap(GLuint &depthFBO, GLuint &depthMap, int SHADOW_WIDTH, int SHADOW_HEIGHT);
void createPointShadowMap(GLuint &depthFBO, GLuint &depthCubemap, int SHADOW_SIZE);
std::vector<glm::mat4> buildPointLightMatrices(glm::vec3 lightPos, float nearPlane, float farPlane);

float deltaTime = 0.0f;	// Time between current frame and last frame
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
const unsigned int POINT_SHADOW_SIZE = 1024;     // cubemap resolution
GLuint pointDepthFBO = 0;                        // depth-only FBO
GLuint pointDepthCubemap = 0;                    // depth cubemap
float pointNear = 1.0f;
float pointFar  = 25.0f;

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




    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Failed to initialize GLAD\n";
        return -1;
    }
//    createShadowMap(depthFBO, depthMap, SHADOW_WIDTH, SHADOW_HEIGHT);
    createPointShadowMap(pointDepthFBO, pointDepthCubemap, POINT_SHADOW_SIZE);

//    glGenFramebuffers(1, &depthFBO);
//
//    glGenTextures(1, &depthMap);
//    glBindTexture(GL_TEXTURE_2D, depthMap);
//    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT,
//                 SHADOW_WIDTH, SHADOW_HEIGHT, 0,
//                 GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
//    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
//    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
//    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
//    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
//
//    float borderColor[] = {1.0, 1.0, 1.0, 1.0};
//    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
//
//    glBindFramebuffer(GL_FRAMEBUFFER, depthFBO);
//    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMap, 0);
//    glDrawBuffer(GL_NONE);
//    glReadBuffer(GL_NONE);
//    glBindFramebuffer(GL_FRAMEBUFFER, 0);


//    glViewport(0, 0, 400, 300);

    std::vector<std::string> faces
            {       std::string(ASSETS_DIR) + "skybox/right.jpg",
                    std::string(ASSETS_DIR) + "skybox/left.jpg",
                    std::string(ASSETS_DIR) + "skybox/top.jpg",
                    std::string(ASSETS_DIR) + "skybox/bottom.jpg",
                    std::string(ASSETS_DIR) + "skybox/front.jpg",
                    std::string(ASSETS_DIR) + "skybox/back.jpg"};

    std::string cube_vertPath = std::string(SHADER_DIR) + "cube_vertex.vert";
    std::string cube_fragPath = std::string(SHADER_DIR) + "cube_fragment.frag";
    ShaderProgram cubeShader(cube_vertPath, cube_fragPath);
    Mesh cube(std::string(ASSETS_DIR) + "box.obj", cubeShader.getID());
    Mesh plane(std::string(ASSETS_DIR) + "plane.obj", cubeShader.getID());
    GLuint cube_diffuse = cubeShader.bindTexture2D("material.diffuse", std::string(ASSETS_DIR) + "container2.png", 0, false);
    GLuint cube_specular = cubeShader.bindTexture2D("material.specular", std::string(ASSETS_DIR) + "container2_specular.png", 1, false);
    GLuint plane_diffuse = cubeShader.bindTexture2D("material.diffuse", std::string(ASSETS_DIR) + "wood.png", 0, false);
    GLuint plane_specular = cubeShader.bindTexture2D("material.specular", std::string(ASSETS_DIR) + "wood_specular.png", 1, false);
    GLuint skyboxID = cubeShader.bindCubeMap("skybox", faces, 2);
    cubeShader.setUniform("material.shininess", 32.0f);
    cubeShader.setUniform("material.alpha",     1.0f);

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
            glm::vec3( 0.0f,  0.0f,  0.0f),
            glm::vec3( 2.0f,  5.0f, -15.0f),
            glm::vec3(-1.5f, -2.2f, -2.5f),
            glm::vec3(-3.8f, -2.0f, -12.3f),
            glm::vec3( 2.4f, -0.4f, -3.5f),
            glm::vec3(-1.7f,  3.0f, -7.5f),
            glm::vec3( 1.3f, -2.0f, -2.5f),
            glm::vec3( 1.5f,  2.0f, -2.5f),
            glm::vec3( 1.5f,  0.2f, -1.5f),
            glm::vec3(-1.3f,  1.0f, -1.5f)
    };

    gfx::SceneConfig cfg = gfx::makeDefaultSceneConfig();

//    cfg.dirLights.resize(1);
    cfg.dirLights.clear();
    cfg.pointLights.resize(1);
//    cfg.pointLights.clear();
    cfg.spotLights.clear();
    gfx::applyDirLights(cubeShader,   cfg.dirLights);
    gfx::applyPointLights(cubeShader, cfg.pointLights);
    gfx::applySpotLights(cubeShader,  cfg.spotLights);

// ============================================================
// Six matrices for point light cubemap rendering
// ============================================================
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

        for (int face = 0; face < 6; face++)
        {
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

            // draw cubes
            for (int i = 0; i < 10; i++)
            {
                model = glm::mat4(1.0f);
                model = glm::translate(model, cubePositions[i]);
                model = glm::rotate(model,
                                    glm::radians(20.0f * i),
                                    glm::vec3(1.0f, 0.3f, 0.5f));

                pointDepthShader.setUniform("model", model);
                cube.draw();
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
        glm::mat4 view       = camera.GetViewMatrix();
        glm::mat4 projection = camera.GetProjection((float)SCR_WIDTH / SCR_HEIGHT);

        cubeShader.setUniform("view",       view);
        cubeShader.setUniform("projection", projection);
        cubeShader.setUniform("viewPos",    camera.Position);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, cube_diffuse);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, cube_specular);

        for (unsigned int i = 0; i < 10; i++){
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, cubePositions[i]);
            float angle = 20.0f * i;
            model = glm::rotate(model, glm::radians(angle), glm::vec3(1.0f, 0.3f, 0.5f));
            cubeShader.setUniform("model", model);
            cube.draw();
        }

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, plane_diffuse);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, plane_specular);
        cubeShader.setUniform("model", glm::mat4(1.0f));
        plane.draw();


        // draw skybox as last
        glDepthFunc(GL_LEQUAL);                 // allow skybox to draw where depth == 1.0
        skyboxShader.use();
        glm::mat4 viewNoTrans = glm::mat4(glm::mat3(view));
        skyboxShader.setUniform("view", viewNoTrans);
        skyboxShader.setUniform("projection", projection);

        // reuse the same cube mesh as skybox geometry
        skybox.draw();
        glDepthFunc(GL_LESS);                  // restore default


//
        lightingCubeShader.use();

        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, cfg.pointLights[0].position);
        model = glm::scale(model, glm::vec3(0.3f));

        lightingCubeShader.setUniform("model", model);
        lightingCubeShader.setUniform("view", view);
        lightingCubeShader.setUniform("projection", projection);

        lightingCube.draw();

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width/2, height/2);
    std::cout<< width << " " << height << std::endl;
}

void mouse_callback(GLFWwindow* window, double xposIn, double yposIn)
{
    float xpos = static_cast<float>(xposIn);
    float ypos = static_cast<float>(yposIn);

    if (camera.firstMouse)
    {
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

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset){
    camera.ProcessMouseScroll(static_cast<float>(yoffset));
}

void createShadowMap(GLuint &depthFBO, GLuint &depthMap,
                     int SHADOW_WIDTH, int SHADOW_HEIGHT)
{
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
                          int SHADOW_SIZE)
{
    glGenFramebuffers(1, &depthFBO);

    glGenTextures(1, &depthCubemap);
    glBindTexture(GL_TEXTURE_CUBE_MAP, depthCubemap);

    for (int i = 0; i < 6; i++)
    {
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
std::vector<glm::mat4> buildPointLightMatrices(glm::vec3 lightPos, float nearPlane, float farPlane)
{
    glm::mat4 shadowProj = glm::perspective(glm::radians(90.0f),
                                            1.0f, nearPlane, farPlane);

    std::vector<glm::mat4> matrices;
    matrices.reserve(6);

    matrices.push_back(shadowProj *
                       glm::lookAt(lightPos, lightPos + glm::vec3( 1, 0, 0), glm::vec3(0,-1,0))); // +X
    matrices.push_back(shadowProj *
                       glm::lookAt(lightPos, lightPos + glm::vec3(-1, 0, 0), glm::vec3(0,-1,0))); // -X
    matrices.push_back(shadowProj *
                       glm::lookAt(lightPos, lightPos + glm::vec3( 0, 1, 0), glm::vec3(0, 0,1))); // +Y
    matrices.push_back(shadowProj *
                       glm::lookAt(lightPos, lightPos + glm::vec3( 0,-1, 0), glm::vec3(0, 0,-1))); // -Y
    matrices.push_back(shadowProj *
                       glm::lookAt(lightPos, lightPos + glm::vec3( 0, 0, 1), glm::vec3(0,-1,0))); // +Z
    matrices.push_back(shadowProj *
                       glm::lookAt(lightPos, lightPos + glm::vec3( 0, 0,-1), glm::vec3(0,-1,0))); // -Z

    return matrices;
}
