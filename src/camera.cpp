//
// Created by dengq on 10/29/25.
//
#include "camera.h"
#include <cmath>

Camera::Camera() {
    updateVectors();
}

Camera::Camera(glm::vec3 position, glm::vec3 worldUp, float yawDeg, float pitchDeg)
    : Position(position), WorldUp(worldUp), Yaw(yawDeg), Pitch(pitchDeg) {
    updateVectors();
}

glm::mat4 Camera::GetViewMatrix() const {
    return glm::lookAt(Position, Position + Front, Up);
}

glm::mat4 Camera::GetProjection(float aspect, float nearPlane, float farPlane) const {
    return glm::perspective(glm::radians(Zoom), aspect, nearPlane, farPlane);
}

void Camera::ProcessKeyboard(GLFWwindow *window, float deltaTime) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
    const float velocity = MovementSpeed * deltaTime;
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        Position += velocity * Front;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        Position -= velocity * Front;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        Position -= glm::normalize(glm::cross(Front, Up)) * velocity;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        Position += glm::normalize(glm::cross(Front, Up)) * velocity;
}

void Camera::ProcessMouseMovement(float xoffset, float yoffset, bool constrainPitch) {
    xoffset *= MouseSensitivity;
    yoffset *= MouseSensitivity;

    Yaw += xoffset;
    Pitch += yoffset;

    if (constrainPitch) {
        Pitch = std::clamp(Pitch, -89.9f, 89.9f);
    }

    updateVectors();
}

void Camera::ProcessMouseScroll(float yoffset) {
    Zoom -= yoffset;
    Zoom = std::clamp(Zoom, 1.0f, 90.0f);
}

void Camera::updateVectors() {
    // Convert yaw/pitch to a forward vector (right-handed, OpenGL-style)
    const float yawRad = glm::radians(Yaw);
    const float pitchRad = glm::radians(Pitch);

    glm::vec3 f;
    f.x = std::cos(yawRad) * std::cos(pitchRad);
    f.y = std::sin(pitchRad);
    f.z = std::sin(yawRad) * std::cos(pitchRad);
    Front = glm::normalize(f);

    // Derive Right and Up to form an orthonormal basis (Gram–Schmidt)
    Right = glm::normalize(glm::cross(Front, WorldUp));
    // If Front is nearly parallel to WorldUp, Right can become tiny; clamping Pitch above avoids this.
    Up = glm::normalize(glm::cross(Right, Front));
}
