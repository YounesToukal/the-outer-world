#pragma once

#include "solar_system.h"

struct GLFWwindow;

// What the overlay needs to know about the application each frame.
struct OverlayState {
    BodyId focusedBody;
    float timeSinceFocusChange;
    float timeScale;
    bool showInfoPanel;
    bool onGround;
    bool comparingSizes;
    BodyId comparedPlanet;
};

// Read-only text panels drawn with Dear ImGui on top of the scene.
class Overlay {
public:
    explicit Overlay(GLFWwindow* window);
    ~Overlay();

    Overlay(const Overlay&) = delete;
    Overlay& operator=(const Overlay&) = delete;

    void draw(const SolarSystem& system, const OverlayState& state) const;
};
