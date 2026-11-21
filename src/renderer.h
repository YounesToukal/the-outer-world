#pragma once

#include <vector>

#include <glm/glm.hpp>

#include "mesh.h"
#include "orbit_lines.h"
#include "saturn_rings.h"
#include "shader.h"
#include "solar_system.h"
#include "texture.h"

class Camera;

struct SceneOptions {
    bool showPlanetOrbits = false;
    bool showSatelliteOrbits = false;
};

// Owns every GPU resource and draws the solar system.
// Shaders and textures are read from shaders/ and assets/textures/ relative to the working directory.
class Renderer {
public:
    explicit Renderer(const SolarSystem& system);

    void beginFrame(const glm::ivec2& framebufferSize) const;

    void drawScene(const SolarSystem& system, const Camera& camera, float aspectRatio,
                   const SceneOptions& options) const;

    // The Sun and another body side by side at their true relative size.
    void drawSizeComparison(const SolarSystem& system, BodyId compared, float aspectRatio) const;

private:
    struct FrameContext {
        glm::mat4 view;
        glm::mat4 projection;
        glm::vec3 cameraPosition;
        glm::vec3 lightPosition;
        float time;
    };

    void drawBody(const CelestialBody& body, const glm::mat4& model, const FrameContext& frame) const;
    void drawRings(const SolarSystem& system, const FrameContext& frame) const;
    void drawSky(const FrameContext& frame) const;
    void drawOrbits(const SolarSystem& system, const FrameContext& frame, const SceneOptions& options) const;

    Shader sunShader_;
    Shader earthShader_;
    Shader planetShader_;
    Shader ringShader_;
    Shader orbitShader_;
    Shader skyShader_;

    Mesh sphere_;
    Mesh cube_;

    std::vector<Texture> surfaceTextures_;  // indexed by BodyId
    Texture earthNightTexture_;
    Texture earthCloudTexture_;
    Texture starfieldTexture_;

    SaturnRings saturnRings_;
    OrbitLines planetOrbits_;
    OrbitLines satelliteOrbits_;
};
