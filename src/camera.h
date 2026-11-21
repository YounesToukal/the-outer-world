#pragma once

#include <glm/glm.hpp>

// Two modes:
//   orbit:  circles a target at a distance; the mouse turns the camera around the target.
//   ground: stands at a fixed eye position; the mouse turns the view direction.
// Yaw and pitch are measured in a reference frame (three axes, the second one pointing up)
// that the application updates every frame, so the camera can turn along with what it follows.
class Camera {
public:
    void orbit(const glm::vec3& target, const glm::mat3& frame, float distance, float minDistance);
    void follow(const glm::vec3& target, const glm::mat3& frame);
    void zoom(float factor);

    void standAt(const glm::vec3& eye, const glm::mat3& frame);

    void setAngles(float yawDegrees, float pitchDegrees);
    void turn(float yawDegrees, float pitchDegrees);

    bool isOnGround() const { return mode_ == Mode::Ground; }
    float distance() const { return distance_; }
    float minDistance() const { return minDistance_; }

    glm::vec3 position() const;
    glm::mat4 viewMatrix() const;
    glm::mat4 projectionMatrix(float aspectRatio) const;

private:
    enum class Mode { Orbit, Ground };

    glm::vec3 direction() const;

    Mode mode_ = Mode::Orbit;
    glm::vec3 target_{0.0f};
    glm::vec3 eye_{0.0f};
    glm::mat3 frame_{1.0f};
    float distance_ = 5.0f;
    float minDistance_ = 1.0f;
    float yaw_ = -90.0f;
    float pitch_ = 0.0f;
};
