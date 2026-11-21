#pragma once

#include <glad/glad.h>

#include "mesh.h"

// Saturn's main rings (D, C, B, A and the Cassini Division) as tens of thousands of small rocks.
// One rock mesh is drawn once per particle with instanced rendering; each instance carries
// its own transform (in the ring plane) and color.
// Instance attribute locations: 3 to 6 transform columns, 7 color.
class SaturnRings {
public:
    SaturnRings(float planetRadius, int maxParticlesPerZone);
    ~SaturnRings();

    SaturnRings(const SaturnRings&) = delete;
    SaturnRings& operator=(const SaturnRings&) = delete;

    void draw() const;

private:
    Mesh rock_;
    GLuint instanceBuffer_ = 0;
    GLsizei particleCount_ = 0;
};
