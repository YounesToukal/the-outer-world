#include "mesh.h"

#include <array>
#include <cmath>
#include <cstddef>
#include <utility>

#include <glm/gtc/constants.hpp>

Mesh::Mesh(const std::vector<Vertex>& vertices, const std::vector<GLuint>& indices)
    : indexCount_(static_cast<GLsizei>(indices.size())) {
    glGenVertexArrays(1, &vertexArray_);
    glGenBuffers(1, &vertexBuffer_);
    glGenBuffers(1, &indexBuffer_);

    glBindVertexArray(vertexArray_);

    glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer_);
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(vertices.size() * sizeof(Vertex)), vertices.data(),
                 GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer_);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLsizeiptr>(indices.size() * sizeof(GLuint)), indices.data(),
                 GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                          reinterpret_cast<const void*>(offsetof(Vertex, position)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                          reinterpret_cast<const void*>(offsetof(Vertex, normal)));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                          reinterpret_cast<const void*>(offsetof(Vertex, texCoord)));

    glBindVertexArray(0);
}

Mesh::~Mesh() {
    release();
}

Mesh::Mesh(Mesh&& other) noexcept
    : vertexArray_(std::exchange(other.vertexArray_, 0)),
      vertexBuffer_(std::exchange(other.vertexBuffer_, 0)),
      indexBuffer_(std::exchange(other.indexBuffer_, 0)),
      indexCount_(std::exchange(other.indexCount_, 0)) {}

Mesh& Mesh::operator=(Mesh&& other) noexcept {
    if (this != &other) {
        release();
        vertexArray_ = std::exchange(other.vertexArray_, 0);
        vertexBuffer_ = std::exchange(other.vertexBuffer_, 0);
        indexBuffer_ = std::exchange(other.indexBuffer_, 0);
        indexCount_ = std::exchange(other.indexCount_, 0);
    }
    return *this;
}

void Mesh::draw() const {
    glBindVertexArray(vertexArray_);
    glDrawElements(GL_TRIANGLES, indexCount_, GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);
}

void Mesh::drawInstanced(GLsizei instanceCount) const {
    glBindVertexArray(vertexArray_);
    glDrawElementsInstanced(GL_TRIANGLES, indexCount_, GL_UNSIGNED_INT, nullptr, instanceCount);
    glBindVertexArray(0);
}

void Mesh::release() {
    glDeleteVertexArrays(1, &vertexArray_);
    glDeleteBuffers(1, &vertexBuffer_);
    glDeleteBuffers(1, &indexBuffer_);
}

Mesh createSphere(int segments, int rings) {
    const auto ringCount = static_cast<std::size_t>(rings);
    const auto segmentCount = static_cast<std::size_t>(segments);

    std::vector<Vertex> vertices;
    vertices.reserve((ringCount + 1) * (segmentCount + 1));

    for (int ring = 0; ring <= rings; ++ring) {
        const float v = static_cast<float>(ring) / static_cast<float>(rings);
        const float polarAngle = v * glm::pi<float>();

        for (int segment = 0; segment <= segments; ++segment) {
            const float u = static_cast<float>(segment) / static_cast<float>(segments);
            const float azimuth = u * glm::two_pi<float>();

            // The azimuth grows westward, so the u coordinate is mirrored.
            const glm::vec3 direction(std::cos(azimuth) * std::sin(polarAngle), std::cos(polarAngle),
                                      std::sin(azimuth) * std::sin(polarAngle));
            vertices.push_back({direction, direction, glm::vec2(1.0f - u, v)});
        }
    }

    std::vector<GLuint> indices;
    indices.reserve(ringCount * segmentCount * 6);
    for (int ring = 0; ring < rings; ++ring) {
        for (int segment = 0; segment < segments; ++segment) {
            const auto current = static_cast<GLuint>(ring * (segments + 1) + segment);
            const auto below = current + static_cast<GLuint>(segments + 1);

            indices.insert(indices.end(), {current, current + 1, below});
            indices.insert(indices.end(), {below, current + 1, below + 1});
        }
    }

    return Mesh(vertices, indices);
}

Mesh createRock(std::mt19937& random) {
    const float t = (1.0f + std::sqrt(5.0f)) / 2.0f;
    std::array<glm::vec3, 12> corners = {{
        {-1, t, 0}, {1, t, 0}, {-1, -t, 0}, {1, -t, 0},
        {0, -1, t}, {0, 1, t}, {0, -1, -t}, {0, 1, -t},
        {t, 0, -1}, {t, 0, 1}, {-t, 0, -1}, {-t, 0, 1},
    }};
    static constexpr std::array<GLuint, 60> kFaces = {
        0, 11, 5,  0, 5, 1,   0, 1, 7,   0, 7, 10,  0, 10, 11,
        1, 5, 9,   5, 11, 4,  11, 10, 2, 10, 7, 6,  7, 1, 8,
        3, 9, 4,   3, 4, 2,   3, 2, 6,   3, 6, 8,   3, 8, 9,
        4, 9, 5,   2, 4, 11,  6, 2, 10,  8, 6, 7,   9, 8, 1,
    };

    std::uniform_real_distribution<float> displacement(0.7f, 1.3f);
    for (glm::vec3& corner : corners) {
        corner = glm::normalize(corner) * displacement(random);
    }

    // Each face gets its own three vertices so that the normals stay flat.
    std::vector<Vertex> vertices;
    std::vector<GLuint> indices;
    vertices.reserve(kFaces.size());
    indices.reserve(kFaces.size());
    for (std::size_t i = 0; i < kFaces.size(); i += 3) {
        const glm::vec3& a = corners[kFaces[i]];
        const glm::vec3& b = corners[kFaces[i + 1]];
        const glm::vec3& c = corners[kFaces[i + 2]];
        const glm::vec3 normal = glm::normalize(glm::cross(b - a, c - a));

        for (const glm::vec3& position : {a, b, c}) {
            indices.push_back(static_cast<GLuint>(vertices.size()));
            vertices.push_back({position, normal, glm::vec2(0.0f)});
        }
    }

    return Mesh(vertices, indices);
}

Mesh createCube() {
    std::vector<Vertex> vertices;
    for (int corner = 0; corner < 8; ++corner) {
        const glm::vec3 position((corner & 1) ? 1.0f : -1.0f, (corner & 2) ? 1.0f : -1.0f, (corner & 4) ? 1.0f : -1.0f);
        vertices.push_back({position, glm::vec3(0.0f), glm::vec2(0.0f)});
    }

    const std::vector<GLuint> indices = {
        0, 2, 1, 1, 2, 3,  // -z
        4, 5, 6, 5, 7, 6,  // +z
        0, 1, 4, 1, 5, 4,  // -y
        2, 6, 3, 3, 6, 7,  // +y
        0, 4, 2, 2, 4, 6,  // -x
        1, 3, 5, 3, 7, 5,  // +x
    };

    return Mesh(vertices, indices);
}
