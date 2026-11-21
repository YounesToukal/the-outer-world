cmake_minimum_required(VERSION 3.16)
project(TheOuterWorld LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

# Dependencies: installed packages are used when present, otherwise they are downloaded.
include(FetchContent)

find_package(OpenGL REQUIRED)

find_package(glfw3 3.3 QUIET)
if(NOT glfw3_FOUND)
    set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_WAYLAND OFF CACHE BOOL "" FORCE)
    FetchContent_Declare(glfw GIT_REPOSITORY https://github.com/glfw/glfw.git GIT_TAG 3.4)
    FetchContent_MakeAvailable(glfw)
endif()

find_package(glm QUIET)
if(NOT glm_FOUND)
    FetchContent_Declare(glm GIT_REPOSITORY https://github.com/g-truc/glm.git GIT_TAG 1.0.1)
    FetchContent_MakeAvailable(glm)
endif()

# Vendored libraries
add_library(glad STATIC third_party/glad/src/glad.c)
target_include_directories(glad SYSTEM PUBLIC third_party/glad/include)

add_library(imgui STATIC
    third_party/imgui/imgui.cpp
    third_party/imgui/imgui_draw.cpp
    third_party/imgui/imgui_tables.cpp
    third_party/imgui/imgui_widgets.cpp
    third_party/imgui/backends/imgui_impl_glfw.cpp
    third_party/imgui/backends/imgui_impl_opengl3.cpp)
target_include_directories(imgui SYSTEM PUBLIC third_party/imgui third_party/imgui/backends)
target_compile_definitions(imgui PUBLIC IMGUI_DISABLE_OBSOLETE_FUNCTIONS)
target_link_libraries(imgui PUBLIC glfw)

add_library(stb INTERFACE)
target_include_directories(stb SYSTEM INTERFACE third_party/stb)

if(MSVC)
    set(PROJECT_WARNINGS /W4 /utf-8)
else()
    set(PROJECT_WARNINGS -Wall -Wextra -Wpedantic -Wshadow)
endif()

# Application
add_executable(the-outer-world
    src/main.cpp
    src/application.cpp
    src/camera.cpp
    src/mesh.cpp
    src/orbit_lines.cpp
    src/overlay.cpp
    src/renderer.cpp
    src/saturn_rings.cpp
    src/shader.cpp
    src/solar_system.cpp
    src/texture.cpp
    src/window.cpp)
target_link_libraries(the-outer-world PRIVATE glad imgui stb glfw glm::glm OpenGL::GL)
target_compile_definitions(the-outer-world PRIVATE GLFW_INCLUDE_NONE)
target_compile_options(the-outer-world PRIVATE ${PROJECT_WARNINGS})
# Shaders and textures are loaded relative to the repository root.
set_target_properties(the-outer-world PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})

# Offline tool that produces assets/textures/starfield.png
add_executable(starfield-generator tools/starfield_generator.cpp)
target_link_libraries(starfield-generator PRIVATE stb)
target_compile_options(starfield-generator PRIVATE ${PROJECT_WARNINGS})
