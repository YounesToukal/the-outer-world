#pragma once

#include <glm/glm.hpp>

struct GLFWwindow;

// Initializes GLFW and owns a full-screen window with an OpenGL 3.3 core context.
class Window {
public:
    explicit Window(const char* title);
    ~Window();

    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;

    GLFWwindow* handle() const { return handle_; }
    glm::ivec2 framebufferSize() const;
    bool shouldClose() const;
    void close();
    void swapBuffers();

private:
    GLFWwindow* handle_ = nullptr;
};
