#ifndef CAMERA_H
#define CAMERA_H

#include <glm.hpp>
#include <gtc/matrix_transform.hpp>
#include <algorithm>
#include <GLFW/glfw3.h>

class Camera {
public:
    // --- Public state ---
    glm::vec3 Position{0.0f, 0.0f, 3.0f};
    glm::vec3 Front{0.0f, 0.0f, -1.0f};
    glm::vec3 Up{0.0f, 1.0f, 0.0f};
    glm::vec3 Right{1.0f, 0.0f, 0.0f};
    glm::vec3 WorldUp{0.0f, 1.0f, 0.0f};

    // Euler angles (degrees)
    float Yaw   = -90.0f;   // facing -Z by default
    float Pitch =   0.0f;
    float lastX;
    float lastY;
    bool firstMouse = true;

    // Tunables
    float MovementSpeed    = 3.0f;   // units / second
    float MouseSensitivity = 0.1f;   // degrees / pixel
    float Zoom             = 45.0f;  // FOV (deg)

    // --- Ctors ---
    Camera();
    Camera(glm::vec3 position, glm::vec3 worldUp, float yawDeg = -90.0f, float pitchDeg = 0.0f);

    // --- Matrices ---
    glm::mat4 GetViewMatrix() const;
    glm::mat4 GetProjection(float aspect, float nearPlane = 0.1f, float farPlane = 100.0f) const;

    // key* are true if pressed; deltaTime in seconds.
    void ProcessKeyboard(GLFWwindow *window, float deltaTime);
    // Mouse deltas in pixels since last frame; positive x = right, positive y = up.
    void ProcessMouseMovement(float xoffset, float yoffset, bool constrainPitch = true);
    // Mouse wheel delta (typical GLFW sign: positive = scroll up = zoom in).
    void ProcessMouseScroll(float yoffset);

private:
    void updateVectors(); // recompute Front/Right/Up from Yaw/Pitch + WorldUp
};
#endif