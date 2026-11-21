#include "application.h"

#include <algorithm>
#include <array>
#include <cmath>

#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>

namespace {

constexpr std::array kFocusOrder = {BodyId::Sun, BodyId::Mercury, BodyId::Venus, BodyId::Earth, BodyId::Mars,
                                    BodyId::Jupiter, BodyId::Saturn, BodyId::Uranus, BodyId::Neptune};
constexpr std::array kComparedPlanets = {BodyId::Mercury, BodyId::Venus, BodyId::Earth, BodyId::Mars,
                                         BodyId::Jupiter, BodyId::Saturn, BodyId::Uranus, BodyId::Neptune};

constexpr float kMouseSensitivity = 0.1f;   // degrees per pixel
constexpr float kZoomRate = 1.0f;           // the distance changes by a factor e per second
constexpr float kMinDistanceFactor = 1.2f;  // closest orbit distance, in radii of the focused body
constexpr float kInitialYaw = 230.0f;       // degrees; 180 looks from the Sun's side, 230 also shows the terminator
constexpr float kInitialPitch = 20.0f;      // degrees above the orbital plane
constexpr float kGroundEyeHeight = 1.01f;   // in Earth radii, measured from Earth's center
constexpr float kMaxTimeScale = 1024.0f;
constexpr float kMaxFrameTime = 0.1f;       // longer pauses do not make the simulation jump

template <std::size_t N>
BodyId nextInCycle(const std::array<BodyId, N>& cycle, BodyId current, int step) {
    const auto position = std::find(cycle.begin(), cycle.end(), current) - cycle.begin();
    const auto count = static_cast<std::ptrdiff_t>(N);
    return cycle[static_cast<std::size_t>((position + step + count) % count)];
}

}  // namespace

Application::Application() : window_("The Outer World"), renderer_(system_), overlay_(window_.handle()) {
    GLFWwindow* handle = window_.handle();
    glfwSetWindowUserPointer(handle, this);
    glfwSetKeyCallback(handle, onKey);
    glfwSetMouseButtonCallback(handle, onMouseButton);
    glfwSetCursorPosCallback(handle, onCursorMove);

    glfwSetInputMode(handle, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    if (glfwRawMouseMotionSupported()) {
        glfwSetInputMode(handle, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
    }

    focusOn(BodyId::Earth);
}

void Application::run() {
    double previousTime = glfwGetTime();
    while (!window_.shouldClose()) {
        glfwPollEvents();

        const double currentTime = glfwGetTime();
        const float deltaTime = std::min(static_cast<float>(currentTime - previousTime), kMaxFrameTime);
        previousTime = currentTime;

        update(deltaTime);
        render();
    }
}

void Application::onKey(GLFWwindow* window, int key, int /*scancode*/, int action, int /*mods*/) {
    if (action == GLFW_PRESS) {
        static_cast<Application*>(glfwGetWindowUserPointer(window))->handleKeyPress(key);
    }
}

void Application::onMouseButton(GLFWwindow* window, int button, int action, int /*mods*/) {
    if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS) {
        static_cast<Application*>(glfwGetWindowUserPointer(window))->selectNextBody(1);
    }
}

void Application::onCursorMove(GLFWwindow* window, double x, double y) {
    static_cast<Application*>(glfwGetWindowUserPointer(window))->handleCursorMove(glm::dvec2(x, y));
}

void Application::handleKeyPress(int key) {
    switch (key) {
    case GLFW_KEY_ESCAPE:
        window_.close();
        break;
    case GLFW_KEY_UP:
        timeScale_ = std::min(timeScale_ * 2.0f, kMaxTimeScale);
        break;
    case GLFW_KEY_DOWN:
        timeScale_ = std::max(timeScale_ / 2.0f, 1.0f);
        break;
    case GLFW_KEY_C:
        comparingSizes_ = !comparingSizes_;
        break;
    case GLFW_KEY_LEFT:
        selectNextBody(-1);
        break;
    case GLFW_KEY_RIGHT:
        selectNextBody(1);
        break;
    case GLFW_KEY_H:
        showSatelliteOrbits_ = !showSatelliteOrbits_;
        break;
    case GLFW_KEY_I:
        showInfoPanel_ = !showInfoPanel_;
        break;
    case GLFW_KEY_X:
        if (camera_.isOnGround()) {
            focusOn(BodyId::Earth);
        }
        break;
    default:
        break;
    }
}

void Application::handleCursorMove(const glm::dvec2& position) {
    if (hasCursorPosition_) {
        const glm::vec2 offset(position - cursorPosition_);
        camera_.turn(offset.x * kMouseSensitivity, -offset.y * kMouseSensitivity);
    }
    cursorPosition_ = position;
    hasCursorPosition_ = true;
}

void Application::focusOn(BodyId id) {
    const CelestialBody& body = system_.body(id);
    focusedBody_ = id;
    timeSinceFocusChange_ = 0.0f;

    camera_.orbit(body.position, sunFacingFrame(body), body.viewDistance, kMinDistanceFactor * body.radius);
    camera_.setAngles(kInitialYaw, kInitialPitch);
}

// Changes the compared planet in the size comparison, and the focused body otherwise.
void Application::selectNextBody(int step) {
    if (comparingSizes_) {
        comparedPlanet_ = nextInCycle(kComparedPlanets, comparedPlanet_, step);
    } else {
        focusOn(nextInCycle(kFocusOrder, focusedBody_, step));
    }
}

// Axes turned around the vertical so that the first axis points away from the Sun.
// An orbiting camera that uses them keeps the same lighting on its target as the target moves.
glm::mat3 Application::sunFacingFrame(const CelestialBody& body) const {
    if (body.id == BodyId::Sun) {
        return glm::mat3(1.0f);
    }
    const glm::vec3 toSun = system_.body(BodyId::Sun).position - body.position;
    const float angle = std::atan2(toSun.z, -toSun.x);
    return glm::mat3(glm::rotate(glm::mat4(1.0f), angle, glm::vec3(0.0f, 1.0f, 0.0f)));
}

void Application::placeCameraOnEarth() {
    // The eye stands above the north pole and turns with Earth, so the sky appears to rotate.
    const CelestialBody& earth = system_.body(BodyId::Earth);
    const glm::mat3 earthAxes(system_.orientation(BodyId::Earth));
    camera_.standAt(earth.position + earthAxes[1] * (kGroundEyeHeight * earth.radius), earthAxes);
}

void Application::update(float deltaTime) {
    system_.update(deltaTime * timeScale_);
    timeSinceFocusChange_ += deltaTime;

    if (camera_.isOnGround()) {
        placeCameraOnEarth();
        return;
    }

    GLFWwindow* handle = window_.handle();
    float zoomDirection = 0.0f;
    if (glfwGetKey(handle, GLFW_KEY_W) == GLFW_PRESS) zoomDirection -= 1.0f;
    if (glfwGetKey(handle, GLFW_KEY_S) == GLFW_PRESS) zoomDirection += 1.0f;

    const CelestialBody& focused = system_.body(focusedBody_);
    camera_.zoom(std::exp(zoomDirection * kZoomRate * deltaTime));
    camera_.follow(focused.position, sunFacingFrame(focused));

    // Zooming all the way into Earth lands on its surface.
    const bool reachedEarth = focusedBody_ == BodyId::Earth && camera_.distance() <= camera_.minDistance();
    if (reachedEarth && zoomDirection < 0.0f) {
        placeCameraOnEarth();
    }
}

void Application::render() {
    const glm::ivec2 size = window_.framebufferSize();
    if (size.x == 0 || size.y == 0) {
        return;  // minimized
    }
    const float aspectRatio = static_cast<float>(size.x) / static_cast<float>(size.y);

    renderer_.beginFrame(size);
    if (comparingSizes_) {
        renderer_.drawSizeComparison(system_, comparedPlanet_, aspectRatio);
    } else {
        const SceneOptions options{
            .showPlanetOrbits = focusedBody_ == BodyId::Sun,
            .showSatelliteOrbits = showSatelliteOrbits_,
        };
        renderer_.drawScene(system_, camera_, aspectRatio, options);
    }

    overlay_.draw(system_, {
        .focusedBody = focusedBody_,
        .timeSinceFocusChange = timeSinceFocusChange_,
        .timeScale = timeScale_,
        .showInfoPanel = showInfoPanel_,
        .onGround = camera_.isOnGround(),
        .comparingSizes = comparingSizes_,
        .comparedPlanet = comparedPlanet_,
    });

    window_.swapBuffers();
}
