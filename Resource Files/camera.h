#ifndef CAMERA_H
#define CAMERA_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

enum Camera_Movement { FORWARD, BACKWARD, LEFT, RIGHT };

class Camera {
public:
    glm::vec3 Position;
    glm::vec3 Front;
    glm::vec3 Up;
    float Yaw;
    float Pitch;
    float Zoom;

    Camera(glm::vec3 position = glm::vec3(0.0f, 0.0f, 0.0f)) : Front(glm::vec3(0.0f, 0.0f, -1.0f)), Yaw(-90.0f), Pitch(0.0f), Zoom(45.0f) {
        Position = position;
        updateCameraVectors();
    }

    glm::mat4 GetViewMatrix() { return glm::lookAt(Position, Position + Front, Up); }

    void ProcessKeyboard(Camera_Movement direction, float deltaTime) {
        float velocity = 2.5f * deltaTime;
        if (direction == FORWARD) Position += Front * velocity;
        if (direction == BACKWARD) Position -= Front * velocity;
        if (direction == LEFT) Position -= glm::normalize(glm::cross(Front, Up)) * velocity;
        if (direction == RIGHT) Position += glm::normalize(glm::cross(Front, Up)) * velocity;
    }

    void ProcessMouseMovement(float xoffset, float yoffset) {
        xoffset *= 0.1f; yoffset *= 0.1f;
        Yaw += xoffset; Pitch += yoffset;
        if (Pitch > 89.0f) Pitch = 89.0f;
        if (Pitch < -89.0f) Pitch = -89.0f;
        updateCameraVectors();
    }

    void ProcessMouseScroll(float yoffset) {
        Zoom -= (float)yoffset;
        if (Zoom < 1.0f) Zoom = 1.0f;
        if (Zoom > 45.0f) Zoom = 45.0f;
    }

private:
    void updateCameraVectors() {
        glm::vec3 front;
        front.x = cos(glm::radians(Yaw)) * cos(glm::radians(Pitch));
        front.y = sin(glm::radians(Pitch));
        front.z = sin(glm::radians(Yaw)) * cos(glm::radians(Pitch));
        Front = glm::normalize(front);
        Up = glm::vec3(0.0f, 1.0f, 0.0f);
    }
};
#endif