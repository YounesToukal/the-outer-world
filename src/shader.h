#pragma once

#include <string>

#include <glad/glad.h>
#include <glm/glm.hpp>

// A linked vertex and fragment shader program.
// The setters write to the program currently in use, so call use() first.
class Shader {
public:
    Shader(const std::string& vertexPath, const std::string& fragmentPath);
    ~Shader();

    Shader(const Shader&) = delete;
    Shader& operator=(const Shader&) = delete;

    void use() const;

    void setInt(const char* name, int value) const;
    void setFloat(const char* name, float value) const;
    void setVec3(const char* name, const glm::vec3& value) const;
    void setMat3(const char* name, const glm::mat3& value) const;
    void setMat4(const char* name, const glm::mat4& value) const;

private:
    GLint location(const char* name) const;

    GLuint program_ = 0;
};
