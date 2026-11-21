#include "window.h"

#include <iostream>
#include <stdexcept>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

namespace {

void printGlfwError(int code, const char* description) {
    std::cerr << "GLFW error " << code << ": " << description << '\n';
}

}  // namespace

Window::Window(const char* title) {
    glfwSetErrorCallback(printGlfwError);
    if (!glfwInit()) {
        throw std::runtime_error("Failed to initialize GLFW");
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
    glfwWindowHint(GLFW_SAMPLES, 4);

    GLFWmonitor* monitor = glfwGetPrimaryMonitor();
    if (monitor == nullptr) {
        glfwTerminate();
        throw std::runtime_error("No monitor found");
    }
    const GLFWvidmode* mode = glfwGetVideoMode(monitor);

    handle_ = glfwCreateWindow(mode->width, mode->height, title, monitor, nullptr);
    if (handle_ == nullptr) {
        glfwTerminate();
        throw std::runtime_error("Failed to create an OpenGL 3.3 window");
    }

    glfwMakeContextCurrent(handle_);
    glfwSwapInterval(1);

    if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress))) {
        glfwDestroyWindow(handle_);
        glfwTerminate();
        throw std::runtime_error("Failed to load OpenGL functions");
    }
}

Window::~Window() {
    glfwDestroyWindow(handle_);
    glfwTerminate();
}

glm::ivec2 Window::framebufferSize() const {
    glm::ivec2 size(0);
    glfwGetFramebufferSize(handle_, &size.x, &size.y);
    return size;
}

bool Window::shouldClose() const {
    return glfwWindowShouldClose(handle_);
}

void Window::close() {
    glfwSetWindowShouldClose(handle_, GLFW_TRUE);
}

void Window::swapBuffers() {
    glfwSwapBuffers(handle_);
}
