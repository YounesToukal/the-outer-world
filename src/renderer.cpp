#include "renderer.h"

#include <array>
#include <random>
#include <string>

#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "camera.h"

namespace {

const std::string kShaderDirectory = "shaders/";
const std::string kTextureDirectory = "assets/textures/";

constexpr int kSphereSegments = 64;
constexpr int kSphereRings = 64;
constexpr int kMaxRingParticlesPerZone = 20000;
constexpr int kOrbitSegments = 200;

constexpr int kSatelliteOrbitCount = 15;
constexpr unsigned kSatelliteOrbitSeed = 3;
constexpr float kPlanetOrbitOpacity = 0.5f;
constexpr float kSatelliteOrbitOpacity = 0.3f;

const glm::vec3 kClearColor(0.01f, 0.01f, 0.03f);

Shader loadShader(const std::string& vertexFile, const std::string& fragmentFile) {
    return Shader(kShaderDirectory + vertexFile, kShaderDirectory + fragmentFile);
}

Texture loadTexture(const std::string& file) {
    return Texture(kTextureDirectory + file);
}

std::vector<Texture> loadSurfaceTextures(const SolarSystem& system) {
    std::vector<Texture> textures;
    textures.reserve(kBodyCount);
    for (const CelestialBody& body : system.bodies()) {
        textures.push_back(loadTexture(body.textureFile));
    }
    return textures;
}

std::vector<OrbitCircle> planetOrbitCircles(const SolarSystem& system) {
    std::vector<OrbitCircle> circles;
    for (const CelestialBody& body : system.bodies()) {
        if (body.parent == BodyId::Sun && body.id != BodyId::Sun) {
            circles.push_back({body.orbitRadius, body.orbitColor, glm::mat4(1.0f)});
        }
    }
    return circles;
}

// Random low and medium orbits around Earth, between 1.3 and 2.2 Earth radii.
std::vector<OrbitCircle> satelliteOrbitCircles(const SolarSystem& system) {
    static constexpr std::array<glm::vec3, 4> kColors{{
        {0.07f, 0.63f, 0.35f},
        {0.70f, 0.28f, 0.07f},
        {0.63f, 0.14f, 0.07f},
        {0.21f, 0.56f, 0.49f},
    }};

    std::mt19937 random(kSatelliteOrbitSeed);
    std::uniform_real_distribution<float> inclination(0.0f, glm::pi<float>());
    std::uniform_real_distribution<float> ascendingNode(0.0f, glm::two_pi<float>());
    std::uniform_real_distribution<float> radius(1.3f, 2.2f);

    const float earthRadius = system.body(BodyId::Earth).radius;
    std::vector<OrbitCircle> circles;
    for (int i = 0; i < kSatelliteOrbitCount; ++i) {
        glm::mat4 plane = glm::rotate(glm::mat4(1.0f), ascendingNode(random), glm::vec3(0.0f, 1.0f, 0.0f));
        plane = glm::rotate(plane, inclination(random), glm::vec3(1.0f, 0.0f, 0.0f));
        circles.push_back({earthRadius * radius(random), kColors[static_cast<std::size_t>(i) % kColors.size()], plane});
    }
    return circles;
}

glm::mat4 placeSphere(const glm::vec3& center, const glm::mat4& orientation, float radius) {
    return glm::scale(glm::translate(glm::mat4(1.0f), center) * orientation, glm::vec3(radius));
}

}  // namespace

Renderer::Renderer(const SolarSystem& system)
    : sunShader_(loadShader("sphere.vert", "sun.frag")),
      earthShader_(loadShader("sphere.vert", "earth.frag")),
      planetShader_(loadShader("sphere.vert", "planet.frag")),
      ringShader_(loadShader("ring.vert", "ring.frag")),
      orbitShader_(loadShader("orbit.vert", "orbit.frag")),
      skyShader_(loadShader("sky.vert", "sky.frag")),
      sphere_(createSphere(kSphereSegments, kSphereRings)),
      cube_(createCube()),
      surfaceTextures_(loadSurfaceTextures(system)),
      earthNightTexture_(loadTexture("earth_night.jpg")),
      earthCloudTexture_(loadTexture("earth_clouds.png")),
      starfieldTexture_(loadTexture("starfield.png")),
      saturnRings_(system.body(BodyId::Saturn).radius, kMaxRingParticlesPerZone),
      planetOrbits_(planetOrbitCircles(system), kOrbitSegments),
      satelliteOrbits_(satelliteOrbitCircles(system), kOrbitSegments) {
    sunShader_.use();
    sunShader_.setInt("uSurfaceTexture", 0);

    planetShader_.use();
    planetShader_.setInt("uSurfaceTexture", 0);

    earthShader_.use();
    earthShader_.setInt("uDayTexture", 0);
    earthShader_.setInt("uNightTexture", 1);
    earthShader_.setInt("uCloudTexture", 2);

    skyShader_.use();
    skyShader_.setInt("uStarfieldTexture", 0);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_MULTISAMPLE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void Renderer::beginFrame(const glm::ivec2& framebufferSize) const {
    glViewport(0, 0, framebufferSize.x, framebufferSize.y);
    glClearColor(kClearColor.r, kClearColor.g, kClearColor.b, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void Renderer::drawScene(const SolarSystem& system, const Camera& camera, float aspectRatio,
                         const SceneOptions& options) const {
    const FrameContext frame{camera.viewMatrix(), camera.projectionMatrix(aspectRatio), camera.position(),
                             system.body(BodyId::Sun).position, system.elapsedTime()};

    for (const CelestialBody& body : system.bodies()) {
        drawBody(body, system.modelMatrix(body.id), frame);
    }
    drawRings(system.ringModelMatrix(), frame);
    drawSky(frame);
    drawOrbits(system, frame, options);
}

void Renderer::drawSizeComparison(const SolarSystem& system, BodyId compared, float aspectRatio) const {
    // One unit is one Earth radius here, so both spheres keep their true proportions.
    const float sunRadius = system.radiusInEarthRadii(BodyId::Sun);
    const glm::vec3 sunCenter(-1.2f * sunRadius, 0.0f, 0.0f);
    const glm::vec3 comparedCenter(0.6f * sunRadius, 0.0f, 0.0f);
    const glm::vec3 eye(0.0f, 0.0f, 1.2f * sunRadius);

    const FrameContext frame{glm::lookAt(eye, glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f)),
                             glm::perspective(glm::radians(65.0f), aspectRatio, 1.0f, 1000.0f), eye, sunCenter,
                             system.elapsedTime()};

    drawBody(system.body(BodyId::Sun), placeSphere(sunCenter, system.orientation(BodyId::Sun), sunRadius), frame);
    drawBody(system.body(compared),
             placeSphere(comparedCenter, system.orientation(compared), system.radiusInEarthRadii(compared)), frame);

    if (compared == BodyId::Saturn) {
        const float scale = system.radiusInEarthRadii(BodyId::Saturn) / system.body(BodyId::Saturn).radius;
        const glm::mat4 placement = glm::translate(glm::mat4(1.0f), comparedCenter) * system.ringOrientation();
        drawRings(glm::scale(placement, glm::vec3(scale)), frame);
    }
}

void Renderer::drawBody(const CelestialBody& body, const glm::mat4& model, const FrameContext& frame) const {
    const Shader& shader = body.id == BodyId::Sun     ? sunShader_
                           : body.id == BodyId::Earth ? earthShader_
                                                      : planetShader_;
    shader.use();
    shader.setMat4("uModel", model);
    shader.setMat4("uView", frame.view);
    shader.setMat4("uProjection", frame.projection);
    shader.setMat3("uNormalMatrix", glm::transpose(glm::inverse(glm::mat3(model))));
    shader.setVec3("uCameraPosition", frame.cameraPosition);
    shader.setVec3("uAtmosphereColor", body.atmosphereColor);

    surfaceTextures_[static_cast<std::size_t>(body.id)].bind(0);
    if (body.id == BodyId::Sun) {
        shader.setFloat("uTime", frame.time);
    } else {
        shader.setVec3("uLightPosition", frame.lightPosition);
    }
    if (body.id == BodyId::Earth) {
        earthNightTexture_.bind(1);
        earthCloudTexture_.bind(2);
    }

    sphere_.draw();
}

void Renderer::drawRings(const glm::mat4& model, const FrameContext& frame) const {
    ringShader_.use();
    ringShader_.setMat4("uModel", model);
    ringShader_.setMat4("uView", frame.view);
    ringShader_.setMat4("uProjection", frame.projection);
    ringShader_.setVec3("uLightPosition", frame.lightPosition);
    saturnRings_.draw();
}

void Renderer::drawSky(const FrameContext& frame) const {
    // Drawn last at maximum depth, so it only fills pixels that nothing else covers.
    glDepthFunc(GL_LEQUAL);
    skyShader_.use();
    skyShader_.setMat4("uView", glm::mat4(glm::mat3(frame.view)));
    skyShader_.setMat4("uProjection", frame.projection);
    starfieldTexture_.bind(0);
    cube_.draw();
    glDepthFunc(GL_LESS);
}

void Renderer::drawOrbits(const SolarSystem& system, const FrameContext& frame, const SceneOptions& options) const {
    if (!options.showPlanetOrbits && !options.showSatelliteOrbits) {
        return;
    }

    // Lines are see-through: they are depth tested but do not hide what is drawn after them.
    glDepthMask(GL_FALSE);
    orbitShader_.use();
    orbitShader_.setMat4("uView", frame.view);
    orbitShader_.setMat4("uProjection", frame.projection);

    if (options.showPlanetOrbits) {
        orbitShader_.setMat4("uModel", glm::translate(glm::mat4(1.0f), system.body(BodyId::Sun).position));
        orbitShader_.setFloat("uOpacity", kPlanetOrbitOpacity);
        planetOrbits_.draw();
    }
    if (options.showSatelliteOrbits) {
        orbitShader_.setMat4("uModel", glm::translate(glm::mat4(1.0f), system.body(BodyId::Earth).position));
        orbitShader_.setFloat("uOpacity", kSatelliteOrbitOpacity);
        satelliteOrbits_.draw();
    }
    glDepthMask(GL_TRUE);
}
