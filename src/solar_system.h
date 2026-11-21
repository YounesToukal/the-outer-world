#pragma once

#include <array>
#include <cstddef>
#include <string>

#include <glm/glm.hpp>

// Order of the bodies in SolarSystem::bodies(). Every body comes after the body it orbits.
enum class BodyId { Sun, Mercury, Venus, Earth, Moon, Mars, Jupiter, Saturn, Uranus, Neptune };

constexpr std::size_t kBodyCount = 10;

// Sizes, distances and speeds are in scene units. They are scaled for viewing:
// the rocky bodies keep their true size relative to Earth (radius 1), the Sun and the
// giant planets are reduced, distances are compressed, and orbits and spins are sped up.
struct CelestialBody {
    BodyId id;
    std::string name;
    std::string textureFile;
    BodyId parent;               // the body it orbits; the Sun is its own parent with orbit radius 0
    float radius;                // scene units
    float radiusKm;              // true equatorial radius, used for size comparisons
    float orbitRadius;           // scene units
    float orbitSpeed;            // radians per simulated second, counterclockwise seen from the north
    float spinSpeed;             // radians per simulated second around the body's own axis
    float axialTilt;             // degrees; values above 90 mean the body spins backwards
    float viewDistance = 0.0f;   // initial camera distance when the body is in focus
    glm::vec3 atmosphereColor;   // rim glow; black for bodies without an atmosphere
    glm::vec3 orbitColor{0.0f};  // color of the orbit line, drawn for the planets
    std::array<std::string, 3> facts{};

    float orbitAngle = 0.0f;
    float spinAngle = 0.0f;
    glm::vec3 position{0.0f};
};

class SolarSystem {
public:
    SolarSystem();

    void update(float deltaTime);

    const std::array<CelestialBody, kBodyCount>& bodies() const { return bodies_; }
    const CelestialBody& body(BodyId id) const { return bodies_[static_cast<std::size_t>(id)]; }
    float elapsedTime() const { return elapsedTime_; }

    // Axial tilt followed by the spin around the body's own axis.
    glm::mat4 orientation(BodyId id) const;
    // Translation, orientation and scale of the unit sphere that represents the body.
    glm::mat4 modelMatrix(BodyId id) const;
    // Saturn's equatorial plane, turning slowly around the planet's axis.
    glm::mat4 ringOrientation() const;
    // The ring plane placed around Saturn.
    glm::mat4 ringModelMatrix() const;
    // Size of a body relative to Earth.
    float radiusInEarthRadii(BodyId id) const;

private:
    std::array<CelestialBody, kBodyCount> bodies_;
    float ringAngle_ = 0.0f;
    float elapsedTime_ = 0.0f;
};
