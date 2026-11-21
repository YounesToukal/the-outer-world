#pragma once

#include <string>

#include <glad/glad.h>

// A mipmapped 2D texture loaded from an image file.
class Texture {
public:
    explicit Texture(const std::string& path);
    ~Texture();

    Texture(Texture&& other) noexcept;
    Texture& operator=(Texture&& other) noexcept;
    Texture(const Texture&) = delete;
    Texture& operator=(const Texture&) = delete;

    void bind(GLuint unit) const;

private:
    GLuint id_ = 0;
};
