#pragma once

#include <glm/glm.hpp>

#include "camera.h"
#include "overlay.h"
#include "renderer.h"
#include "solar_system.h"
#include "window.h"

// Runs the main loop and turns keyboard and mouse input into camera moves and display modes.
class Application {
public:
    Application();

    Application(const Application&) = delete;
    Application& operator=(const Application&) = delete;

    void run();

private:
    static void onKey(GLFWwindow* window, int key, int scancode, int action, int mods);
    static void onMouseButton(GLFWwindow* window, int button, int action, int mods);
    static void onCursorMove(GLFWwindow* window, double x, double y);

    void handleKeyPress(int key);
    void handleCursorMove(const glm::dvec2& position);

    void focusOn(BodyId id);
    void focusNextBody();
    void compareNextBody(int step);
    void placeCameraOnEarth();
    glm::mat3 sunFacingFrame(const CelestialBody& body) const;

    void update(float deltaTime);
    void render();

    Window window_;
    SolarSystem system_;
    Renderer renderer_;
    Overlay overlay_;
    Camera camera_;

    BodyId focusedBody_ = BodyId::Earth;
    BodyId comparedBody_ = BodyId::Earth;
    float timeSinceFocusChange_ = 0.0f;
    float timeScale_ = 1.0f;
    bool showInfoPanel_ = true;
    bool showSatelliteOrbits_ = false;
    bool comparingSizes_ = false;

    bool hasCursorPosition_ = false;
    glm::dvec2 cursorPosition_{0.0};
};
