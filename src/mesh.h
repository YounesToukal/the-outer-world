#pragma once

#include <random>
#include <vector>

#include <glad/glad.h>
#include <glm/glm.hpp>

struct Vertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 texCoord;
};

// Indexed triangle mesh stored on the GPU.
// Attribute locations: 0 position, 1 normal, 2 texture coordinate.
class Mesh {
public:
    Mesh(const std::vector<Vertex>& vertices, const std::vector<GLuint>& indices);
    ~Mesh();

    Mesh(Mesh&& other) noexcept;
    Mesh& operator=(Mesh&& other) noexcept;
    Mesh(const Mesh&) = delete;
    Mesh& operator=(const Mesh&) = delete;

    GLuint vertexArray() const { return vertexArray_; }

    void draw() const;
    void drawInstanced(GLsizei instanceCount) const;

private:
    void release();

    GLuint vertexArray_ = 0;
    GLuint vertexBuffer_ = 0;
    GLuint indexBuffer_ = 0;
    GLsizei indexCount_ = 0;
};

// Unit sphere built from latitude rings and longitude segments. Texture coordinates follow
// the equirectangular layout of planetary maps: north pole at v = 0, longitude 0 at u = 0.5.
Mesh createSphere(int segments, int rings);

// Icosahedron of unit size with randomly displaced vertices and flat normals.
Mesh createRock(std::mt19937& random);

// Cube spanning [-1, 1] on each axis.
Mesh createCube();
