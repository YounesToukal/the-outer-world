#include "saturn_rings.h"

#include <array>
#include <cmath>
#include <cstddef>
#include <random>
#include <vector>

#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace {

constexpr unsigned kRockShapeSeed = 7;
constexpr unsigned kParticleSeed = 11;
constexpr float kRingThickness = 0.015f;  // standard deviation of the height above the ring plane

struct RingZone {
    float innerRadius;  // in planet radii
    float outerRadius;  // in planet radii
    float density;      // fraction of the maximum particle count
    glm::vec3 tint;
    float minParticleSize;
    float maxParticleSize;
};

// Radii follow the observed ring boundaries.
constexpr std::array<RingZone, 5> kZones{{
    {1.11f, 1.23f, 0.05f, {0.60f, 0.60f, 0.65f}, 0.015f, 0.035f},  // D ring
    {1.24f, 1.52f, 0.25f, {0.65f, 0.60f, 0.55f}, 0.020f, 0.050f},  // C ring
    {1.52f, 1.95f, 0.95f, {0.90f, 0.92f, 0.95f}, 0.025f, 0.060f},  // B ring
    {1.95f, 2.02f, 0.03f, {0.40f, 0.35f, 0.30f}, 0.010f, 0.020f},  // Cassini Division
    {2.02f, 2.27f, 0.65f, {0.85f, 0.87f, 0.90f}, 0.020f, 0.055f},  // A ring
}};

constexpr glm::vec3 kIceColor{0.95f, 0.97f, 1.00f};
constexpr glm::vec3 kRockColor{0.55f, 0.50f, 0.45f};

struct RingParticle {
    glm::mat4 transform;
    glm::vec3 color;
};

std::vector<RingParticle> generateParticles(float planetRadius, int maxParticlesPerZone) {
    std::mt19937 random(kParticleSeed);
    std::uniform_real_distribution<float> unit(0.0f, 1.0f);
    std::normal_distribution<float> gaussian(0.0f, 1.0f);

    std::vector<RingParticle> particles;
    for (const RingZone& zone : kZones) {
        const int count = static_cast<int>(static_cast<float>(maxParticlesPerZone) * zone.density);
        for (int i = 0; i < count; ++i) {
            const float angle = unit(random) * glm::two_pi<float>();
            const float radius = planetRadius * glm::mix(zone.innerRadius, zone.outerRadius, unit(random));
            const float height = kRingThickness * gaussian(random);
            const float size = glm::mix(zone.minParticleSize, zone.maxParticleSize, unit(random));
            const glm::vec3 axis = glm::normalize(glm::vec3(gaussian(random), gaussian(random), gaussian(random)));

            glm::mat4 transform = glm::translate(glm::mat4(1.0f),
                                                 glm::vec3(radius * std::cos(angle), height, radius * std::sin(angle)));
            transform = glm::rotate(transform, unit(random) * glm::two_pi<float>(), axis);
            transform = glm::scale(transform, glm::vec3(size));

            // Larger particles are icier and brighter; every particle carries a hint of its zone's tint.
            const float iceFraction = std::sqrt(size / zone.maxParticleSize);
            const glm::vec3 color = glm::mix(glm::mix(kRockColor, kIceColor, iceFraction), zone.tint, 0.3f);

            particles.push_back({transform, color});
        }
    }
    return particles;
}

}  // namespace

SaturnRings::SaturnRings(float planetRadius, int maxParticlesPerZone)
    : rock_([] {
          std::mt19937 random(kRockShapeSeed);
          return createRock(random);
      }()) {
    const std::vector<RingParticle> particles = generateParticles(planetRadius, maxParticlesPerZone);
    particleCount_ = static_cast<GLsizei>(particles.size());

    glGenBuffers(1, &instanceBuffer_);
    glBindVertexArray(rock_.vertexArray());
    glBindBuffer(GL_ARRAY_BUFFER, instanceBuffer_);
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(particles.size() * sizeof(RingParticle)),
                 particles.data(), GL_STATIC_DRAW);

    constexpr GLuint kTransformLocation = 3;
    constexpr GLuint kColorLocation = 7;
    for (GLuint column = 0; column < 4; ++column) {
        const GLuint location = kTransformLocation + column;
        glEnableVertexAttribArray(location);
        glVertexAttribPointer(location, 4, GL_FLOAT, GL_FALSE, sizeof(RingParticle),
                              reinterpret_cast<const void*>(offsetof(RingParticle, transform) +
                                                            column * sizeof(glm::vec4)));
        glVertexAttribDivisor(location, 1);
    }
    glEnableVertexAttribArray(kColorLocation);
    glVertexAttribPointer(kColorLocation, 3, GL_FLOAT, GL_FALSE, sizeof(RingParticle),
                          reinterpret_cast<const void*>(offsetof(RingParticle, color)));
    glVertexAttribDivisor(kColorLocation, 1);

    glBindVertexArray(0);
}

SaturnRings::~SaturnRings() {
    glDeleteBuffers(1, &instanceBuffer_);
}

void SaturnRings::draw() const {
    rock_.drawInstanced(particleCount_);
}
