#include "orbit_lines.h"

#include <cmath>
#include <cstddef>

#include <glm/gtc/constants.hpp>

namespace {

struct LineVertex {
    glm::vec3 position;
    glm::vec3 color;
};

}  // namespace

OrbitLines::OrbitLines(const std::vector<OrbitCircle>& circles, int segmentsPerCircle) {
    std::vector<LineVertex> vertices;
    for (const OrbitCircle& circle : circles) {
        firstVertices_.push_back(static_cast<GLint>(vertices.size()));
        vertexCounts_.push_back(segmentsPerCircle);

        for (int segment = 0; segment < segmentsPerCircle; ++segment) {
            const float angle = glm::two_pi<float>() * static_cast<float>(segment) / static_cast<float>(segmentsPerCircle);
            const glm::vec4 point(circle.radius * std::cos(angle), 0.0f, circle.radius * std::sin(angle), 1.0f);
            vertices.push_back({glm::vec3(circle.plane * point), circle.color});
        }
    }

    glGenVertexArrays(1, &vertexArray_);
    glGenBuffers(1, &vertexBuffer_);

    glBindVertexArray(vertexArray_);
    glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer_);
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(vertices.size() * sizeof(LineVertex)), vertices.data(),
                 GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(LineVertex),
                          reinterpret_cast<const void*>(offsetof(LineVertex, position)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(LineVertex),
                          reinterpret_cast<const void*>(offsetof(LineVertex, color)));

    glBindVertexArray(0);
}

OrbitLines::~OrbitLines() {
    glDeleteVertexArrays(1, &vertexArray_);
    glDeleteBuffers(1, &vertexBuffer_);
}

void OrbitLines::draw() const {
    glBindVertexArray(vertexArray_);
    glMultiDrawArrays(GL_LINE_LOOP, firstVertices_.data(), vertexCounts_.data(),
                      static_cast<GLsizei>(firstVertices_.size()));
    glBindVertexArray(0);
}
