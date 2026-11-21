#include "camera.h"

#include <algorithm>
#include <cmath>

#include <glm/gtc/matrix_transform.hpp>

namespace {

constexpr float kMaxDistance = 300.0f;
constexpr float kMaxPitch = 89.0f;
constexpr float kFieldOfView = 45.0f;
constexpr float kOrbitNearPlane = 0.1f;
constexpr float kGroundNearPlane = 0.002f;  // the ground is only a hundredth of a radius below the eye
constexpr float kFarPlane = 1000.0f;

}  // namespace

void Camera::orbit(const glm::vec3& target, const glm::mat3& frame, float distance, float minDistance) {
    mode_ = Mode::Orbit;
    minDistance_ = minDistance;
    distance_ = std::clamp(distance, minDistance_, kMaxDistance);
    follow(target, frame);
}

void Camera::follow(const glm::vec3& target, const glm::mat3& frame) {
    target_ = target;
    frame_ = frame;
}

void Camera::zoom(float factor) {
    distance_ = std::clamp(distance_ * factor, minDistance_, kMaxDistance);
}

void Camera::standAt(const glm::vec3& eye, const glm::mat3& frame) {
    mode_ = Mode::Ground;
    eye_ = eye;
    frame_ = frame;
}

void Camera::setAngles(float yawDegrees, float pitchDegrees) {
    yaw_ = yawDegrees;
    pitch_ = std::clamp(pitchDegrees, -kMaxPitch, kMaxPitch);
}

void Camera::turn(float yawDegrees, float pitchDegrees) {
    yaw_ += yawDegrees;
    pitch_ = std::clamp(pitch_ + pitchDegrees, -kMaxPitch, kMaxPitch);
}

glm::vec3 Camera::position() const {
    return mode_ == Mode::Orbit ? target_ + distance_ * direction() : eye_;
}

glm::mat4 Camera::viewMatrix() const {
    const glm::vec3 up = frame_[1];
    if (mode_ == Mode::Orbit) {
        return glm::lookAt(position(), target_, up);
    }
    return glm::lookAt(eye_, eye_ + direction(), up);
}

glm::mat4 Camera::projectionMatrix(float aspectRatio) const {
    const float nearPlane = mode_ == Mode::Orbit ? kOrbitNearPlane : kGroundNearPlane;
    return glm::perspective(glm::radians(kFieldOfView), aspectRatio, nearPlane, kFarPlane);
}

glm::vec3 Camera::direction() const {
    const float yaw = glm::radians(yaw_);
    const float pitch = glm::radians(pitch_);
    const glm::vec3 local(std::cos(yaw) * std::cos(pitch), std::sin(pitch), std::sin(yaw) * std::cos(pitch));
    return frame_ * local;
}
