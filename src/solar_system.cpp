#include "solar_system.h"

#include <cassert>
#include <cmath>

#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace {

constexpr float kRingSpinSpeed = 0.02f;

std::size_t indexOf(BodyId id) {
    return static_cast<std::size_t>(id);
}

float wrapAngle(float angle) {
    return std::fmod(angle, glm::two_pi<float>());
}

glm::mat4 tiltAndSpin(float axialTilt, float spinAngle) {
    const glm::mat4 tilt = glm::rotate(glm::mat4(1.0f), glm::radians(axialTilt), glm::vec3(0.0f, 0.0f, 1.0f));
    return glm::rotate(tilt, spinAngle, glm::vec3(0.0f, 1.0f, 0.0f));
}

std::array<CelestialBody, kBodyCount> makeBodies() {
    return {{
        {
            .id = BodyId::Sun,
            .name = "Sun",
            .textureFile = "sun.jpg",
            .parent = BodyId::Sun,
            .radius = 15.0f,
            .radiusKm = 695700.0f,
            .orbitRadius = 0.0f,
            .orbitSpeed = 0.0f,
            .spinSpeed = 0.05f,
            .axialTilt = 7.25f,
            .viewDistance = 40.0f,
            .atmosphereColor = {1.0f, 0.85f, 0.6f},
            .facts = {"Contains 99.86% of solar system mass",
                      "Surface temperature: 5,500°C",
                      "Core temperature: 15 million°C"},
        },
        {
            .id = BodyId::Mercury,
            .name = "Mercury",
            .textureFile = "mercury.jpg",
            .parent = BodyId::Sun,
            .radius = 0.38f,
            .radiusKm = 2440.5f,
            .orbitRadius = 35.0f,
            .orbitSpeed = 0.4f,
            .spinSpeed = 0.01f,
            .axialTilt = 0.03f,
            .viewDistance = 2.0f,
            .atmosphereColor = {0.0f, 0.0f, 0.0f},
            .orbitColor = {0.7f, 0.6f, 0.5f},
            .facts = {"Smallest planet in solar system",
                      "One day = 59 Earth days",
                      "No atmosphere or moons"},
        },
        {
            .id = BodyId::Venus,
            .name = "Venus",
            .textureFile = "venus.jpg",
            .parent = BodyId::Sun,
            .radius = 0.95f,
            .radiusKm = 6051.8f,
            .orbitRadius = 50.0f,
            .orbitSpeed = 0.3f,
            .spinSpeed = 0.005f,
            .axialTilt = 177.36f,
            .viewDistance = 3.0f,
            .atmosphereColor = {0.9f, 0.7f, 0.4f},
            .orbitColor = {0.9f, 0.7f, 0.4f},
            .facts = {"Hottest planet (465°C average)",
                      "Rotates backwards (retrograde)",
                      "Day longer than year (243 vs 225 days)"},
        },
        {
            .id = BodyId::Earth,
            .name = "Earth",
            .textureFile = "earth_day.jpg",
            .parent = BodyId::Sun,
            .radius = 1.0f,
            .radiusKm = 6378.1f,
            .orbitRadius = 65.0f,
            .orbitSpeed = 0.15f,
            .spinSpeed = 0.1f,
            .axialTilt = 23.44f,
            .viewDistance = 3.5f,
            .atmosphereColor = {0.18f, 0.35f, 0.75f},
            .orbitColor = {0.3f, 0.6f, 0.9f},
            .facts = {"Only known planet with life",
                      "71% covered by water",
                      "Atmosphere: 78% nitrogen, 21% oxygen"},
        },
        {
            .id = BodyId::Moon,
            .name = "Moon",
            .textureFile = "moon.jpg",
            .parent = BodyId::Earth,
            .radius = 0.273f,
            .radiusKm = 1738.1f,
            .orbitRadius = 6.0f,
            .orbitSpeed = 0.1f,
            .spinSpeed = 0.1f,  // equal to the orbit speed: the same face always points at Earth
            .axialTilt = 1.54f,
            .atmosphereColor = {0.0f, 0.0f, 0.0f},
        },
        {
            .id = BodyId::Mars,
            .name = "Mars",
            .textureFile = "mars.jpg",
            .parent = BodyId::Sun,
            .radius = 0.53f,
            .radiusKm = 3396.2f,
            .orbitRadius = 80.0f,
            .orbitSpeed = 0.12f,
            .spinSpeed = 0.015f,
            .axialTilt = 25.19f,
            .viewDistance = 2.5f,
            .atmosphereColor = {0.8f, 0.3f, 0.2f},
            .orbitColor = {0.8f, 0.4f, 0.2f},
            .facts = {"Home to largest volcano (Olympus Mons)",
                      "Has polar ice caps",
                      "Day length: 24.6 hours (similar to Earth)"},
        },
        {
            .id = BodyId::Jupiter,
            .name = "Jupiter",
            .textureFile = "jupiter.jpg",
            .parent = BodyId::Sun,
            .radius = 5.0f,
            .radiusKm = 71492.0f,
            .orbitRadius = 110.0f,
            .orbitSpeed = 0.08f,
            .spinSpeed = 0.03f,
            .axialTilt = 3.13f,
            .viewDistance = 12.0f,
            .atmosphereColor = {0.8f, 0.7f, 0.6f},
            .orbitColor = {0.8f, 0.7f, 0.5f},
            .facts = {"Largest planet in solar system",
                      "Great Red Spot: storm 3x Earth's size",
                      "Has 95 known moons"},
        },
        {
            .id = BodyId::Saturn,
            .name = "Saturn",
            .textureFile = "saturn.jpg",
            .parent = BodyId::Sun,
            .radius = 4.5f,
            .radiusKm = 60268.0f,
            .orbitRadius = 140.0f,
            .orbitSpeed = 0.06f,
            .spinSpeed = 0.025f,
            .axialTilt = 26.73f,
            .viewDistance = 15.0f,
            .atmosphereColor = {0.9f, 0.8f, 0.6f},
            .orbitColor = {0.9f, 0.8f, 0.6f},
            .facts = {"Least dense planet (would float in water)",
                      "Rings made of ice and rock particles",
                      "Has 146 known moons"},
        },
        {
            .id = BodyId::Uranus,
            .name = "Uranus",
            .textureFile = "uranus.jpg",
            .parent = BodyId::Sun,
            .radius = 2.5f,
            .radiusKm = 25559.0f,
            .orbitRadius = 170.0f,
            .orbitSpeed = 0.04f,
            .spinSpeed = 0.02f,
            .axialTilt = 97.77f,
            .viewDistance = 7.0f,
            .atmosphereColor = {0.6f, 0.8f, 0.9f},
            .orbitColor = {0.6f, 0.8f, 0.9f},
            .facts = {"Rotates on its side (98° tilt)",
                      "Coldest planet (-224°C)",
                      "Has 13 faint rings"},
        },
        {
            .id = BodyId::Neptune,
            .name = "Neptune",
            .textureFile = "neptune.jpg",
            .parent = BodyId::Sun,
            .radius = 2.4f,
            .radiusKm = 24764.0f,
            .orbitRadius = 200.0f,
            .orbitSpeed = 0.03f,
            .spinSpeed = 0.018f,
            .axialTilt = 28.32f,
            .viewDistance = 7.0f,
            .atmosphereColor = {0.4f, 0.5f, 0.9f},
            .orbitColor = {0.4f, 0.5f, 0.9f},
            .facts = {"Strongest winds (2,100 km/h)",
                      "Farthest planet from Sun",
                      "Has 16 known moons"},
        },
    }};
}

}  // namespace

SolarSystem::SolarSystem() : bodies_(makeBodies()) {
    for (std::size_t i = 0; i < bodies_.size(); ++i) {
        assert(indexOf(bodies_[i].id) == i && "bodies must follow the order of BodyId");
        assert(indexOf(bodies_[i].parent) <= i && "a body must come after the body it orbits");
    }
    update(0.0f);
}

void SolarSystem::update(float deltaTime) {
    elapsedTime_ += deltaTime;

    for (CelestialBody& current : bodies_) {
        current.orbitAngle = wrapAngle(current.orbitAngle + current.orbitSpeed * deltaTime);
        current.spinAngle = wrapAngle(current.spinAngle + current.spinSpeed * deltaTime);

        const glm::vec3 orbitDirection(std::cos(current.orbitAngle), 0.0f, -std::sin(current.orbitAngle));
        current.position = bodies_[indexOf(current.parent)].position + current.orbitRadius * orbitDirection;
    }

    ringAngle_ = wrapAngle(ringAngle_ + kRingSpinSpeed * deltaTime);
}

glm::mat4 SolarSystem::orientation(BodyId id) const {
    const CelestialBody& selected = body(id);
    return tiltAndSpin(selected.axialTilt, selected.spinAngle);
}

glm::mat4 SolarSystem::modelMatrix(BodyId id) const {
    const CelestialBody& selected = body(id);
    const glm::mat4 translation = glm::translate(glm::mat4(1.0f), selected.position);
    return glm::scale(translation * orientation(id), glm::vec3(selected.radius));
}

glm::mat4 SolarSystem::ringOrientation() const {
    return tiltAndSpin(body(BodyId::Saturn).axialTilt, ringAngle_);
}

glm::mat4 SolarSystem::ringModelMatrix() const {
    return glm::translate(glm::mat4(1.0f), body(BodyId::Saturn).position) * ringOrientation();
}

float SolarSystem::radiusInEarthRadii(BodyId id) const {
    return body(id).radiusKm / body(BodyId::Earth).radiusKm;
}
