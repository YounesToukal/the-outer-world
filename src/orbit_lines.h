#pragma once

#include <vector>

#include <glad/glad.h>
#include <glm/glm.hpp>

struct OrbitCircle {
    float radius;
    glm::vec3 color;
    glm::mat4 plane;  // rotates the circle out of the XZ plane
};

// A set of circular orbits drawn as separate line loops.
// Attribute locations: 0 position, 1 color.
class OrbitLines {
public:
    OrbitLines(const std::vector<OrbitCircle>& circles, int segmentsPerCircle);
    ~OrbitLines();

    OrbitLines(const OrbitLines&) = delete;
    OrbitLines& operator=(const OrbitLines&) = delete;

    void draw() const;

private:
    GLuint vertexArray_ = 0;
    GLuint vertexBuffer_ = 0;
    std::vector<GLint> firstVertices_;
    std::vector<GLsizei> vertexCounts_;
};
