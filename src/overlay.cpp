#include "overlay.h"

#include <algorithm>
#include <string>

#include <GLFW/glfw3.h>
#include <glm/gtc/constants.hpp>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

namespace {

constexpr float kNameDisplayDuration = 3.0f;  // seconds the name of a newly focused body stays visible
constexpr float kNameFadeStart = 0.7f;        // fraction of that time after which the name fades out
constexpr float kMargin = 10.0f;
constexpr float kReferenceScreenHeight = 1080.0f;  // panels keep their size up to this height and grow beyond it

constexpr ImGuiWindowFlags kFixedPanel =
    ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse;

float uiScale() {
    return ImGui::GetStyle().FontScaleMain;
}

// The pivot selects the point of the panel placed at the position: (0, 0) top left, (1, 1) bottom right.
// A width of 0 sizes the panel to its content.
void beginPanel(const char* id, ImVec2 position, ImVec2 pivot, float backgroundAlpha, float width,
                ImGuiWindowFlags extraFlags = 0) {
    ImGui::SetNextWindowPos(position, ImGuiCond_Always, pivot);
    ImGui::SetNextWindowBgAlpha(backgroundAlpha);
    if (width > 0.0f) {
        ImGui::SetNextWindowSize(ImVec2(width * uiScale(), 0.0f), ImGuiCond_Always);
    }
    ImGui::Begin(id, nullptr, kFixedPanel | extraFlags);
}

void scaledText(const ImVec4& color, float scale, const char* text) {
    ImGui::PushFont(nullptr, ImGui::GetStyle().FontSizeBase * scale);
    ImGui::TextColored(color, "%s", text);
    ImGui::PopFont();
}

void drawGroundPanel(ImVec2 screen) {
    beginPanel("##GroundView", ImVec2(screen.x * 0.5f, 50.0f * uiScale()), ImVec2(0.5f, 0.0f), 0.7f, 0.0f,
               ImGuiWindowFlags_AlwaysAutoResize);

    scaledText(ImVec4(0.3f, 1.0f, 0.5f, 1.0f), 2.0f, "GROUND VIEW");
    ImGui::Spacing();
    ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.8f, 1.0f), "Look up to see the cosmos");
    ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.8f, 1.0f), "Press X to return to orbit");

    ImGui::End();
}

void drawBodyName(const CelestialBody& body, float timeSinceFocusChange, ImVec2 screen) {
    const float fadeStart = kNameDisplayDuration * kNameFadeStart;
    const float fade = std::clamp(
        1.0f - (timeSinceFocusChange - fadeStart) / (kNameDisplayDuration - fadeStart), 0.0f, 1.0f);

    beginPanel("##PlanetName", ImVec2(screen.x * 0.5f, screen.y * 0.85f), ImVec2(0.5f, 0.5f), 0.75f * fade, 0.0f,
               ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoScrollbar);

    scaledText(ImVec4(0.7f, 0.9f, 1.0f, fade), 2.5f, body.name.c_str());
    ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.8f, fade * 0.8f), "Radius: %.2f units", body.radius);

    ImGui::End();
}

void drawInfoPanel(const SolarSystem& system, const CelestialBody& body, ImVec2 screen) {
    const float margin = kMargin * uiScale();
    beginPanel("##PlanetInfo", ImVec2(screen.x - margin, screen.y - margin), ImVec2(1.0f, 1.0f), 0.85f, 400.0f);

    scaledText(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), 1.8f, body.name.c_str());
    ImGui::Separator();
    ImGui::Spacing();

    const float sizeVsEarth = system.radiusInEarthRadii(body.id);
    ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.3f, 1.0f), "Size vs Earth:");
    ImGui::SameLine();
    if (sizeVsEarth >= 1.0f) {
        ImGui::TextColored(ImVec4(0.9f, 0.9f, 0.9f, 1.0f), "%.1fx larger", sizeVsEarth);
    } else {
        ImGui::TextColored(ImVec4(0.9f, 0.9f, 0.9f, 1.0f), "%.1fx smaller", 1.0f / sizeVsEarth);
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    ImGui::TextColored(ImVec4(0.5f, 1.0f, 0.7f, 1.0f), "Interesting Facts:");
    ImGui::Spacing();
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.95f, 0.95f, 0.95f, 1.0f));
    for (const std::string& fact : body.facts) {
        ImGui::BulletText("%s", fact.c_str());
    }
    ImGui::PopStyleColor();

    ImGui::End();
}

void drawTimeWarpPanel(const SolarSystem& system, float timeScale, ImVec2 screen) {
    const float margin = kMargin * uiScale();
    beginPanel("##TimeSpeed", ImVec2(screen.x - margin, margin), ImVec2(1.0f, 0.0f), 0.75f, 250.0f);

    scaledText(ImVec4(1.0f, 0.7f, 0.2f, 1.0f), 1.3f, "TIME WARP");
    ImGui::Separator();
    ImGui::Spacing();

    // One Earth day is one turn of Earth around its axis.
    const float daysPerSecond = system.body(BodyId::Earth).spinSpeed * timeScale / glm::two_pi<float>();
    const float hoursPerSecond = daysPerSecond * 24.0f;
    const float minutesPerSecond = hoursPerSecond * 60.0f;
    const ImVec4 rateColor(1.0f, 1.0f, 0.5f, 1.0f);

    ImGui::PushFont(nullptr, ImGui::GetStyle().FontSizeBase * 1.8f);
    if (daysPerSecond < 1.0f / 24.0f) {
        if (hoursPerSecond < 1.0f / 60.0f) {
            if (minutesPerSecond < 1.0f) {
                ImGui::TextColored(rateColor, "%.1f sec/sec", minutesPerSecond * 60.0f);
            } else {
                ImGui::TextColored(rateColor, "%.1f min/sec", minutesPerSecond);
            }
        } else {
            ImGui::TextColored(rateColor, "%.1f hrs/sec", hoursPerSecond);
        }
    } else if (daysPerSecond < 30.0f) {
        ImGui::TextColored(rateColor, "%.1f days/sec", daysPerSecond);
    } else if (daysPerSecond < 365.0f) {
        ImGui::TextColored(rateColor, "%.1f mo/sec", daysPerSecond / 30.0f);
    } else {
        ImGui::TextColored(rateColor, "%.2f yr/sec", daysPerSecond / 365.25f);
    }
    ImGui::PopFont();

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();
    ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "UP Faster | DOWN Slower");

    ImGui::End();
}

void drawComparisonPanel(const SolarSystem& system, BodyId planet, ImVec2 screen) {
    beginPanel("##SizeComparison", ImVec2(screen.x * 0.5f, screen.y - kMargin * uiScale()), ImVec2(0.5f, 1.0f),
               0.85f, 500.0f);

    scaledText(ImVec4(1.0f, 0.7f, 0.2f, 1.0f), 1.5f, "SIZE COMPARISON");
    ImGui::Separator();
    ImGui::Spacing();

    const char* name = system.body(planet).name.c_str();
    ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "Sun vs %s", name);
    ImGui::Spacing();
    ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.5f, 1.0f), "Sun is %.1fx larger than %s",
                       system.radiusInEarthRadii(BodyId::Sun) / system.radiusInEarthRadii(planet), name);

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();
    ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "LEFT/RIGHT Change Planet | C Exit Comparison");

    ImGui::End();
}

}  // namespace

Overlay::Overlay(GLFWwindow* window) {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::GetIO().IniFilename = nullptr;

    ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 8.0f;
    style.FrameRounding = 4.0f;
    style.WindowBorderSize = 1.0f;
    style.Colors[ImGuiCol_WindowBg] = ImVec4(0.02f, 0.02f, 0.08f, 0.85f);
    style.Colors[ImGuiCol_Text] = ImVec4(0.9f, 0.95f, 1.0f, 1.0f);

    int windowWidth = 0;
    int windowHeight = 0;
    glfwGetWindowSize(window, &windowWidth, &windowHeight);
    const float scale = std::max(1.0f, static_cast<float>(windowHeight) / kReferenceScreenHeight);
    style.ScaleAllSizes(scale);
    style.FontScaleMain = scale;

    // The panels never take input, so ImGui does not install its own GLFW callbacks.
    ImGui_ImplGlfw_InitForOpenGL(window, false);
    ImGui_ImplOpenGL3_Init("#version 330 core");
}

Overlay::~Overlay() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

void Overlay::draw(const SolarSystem& system, const OverlayState& state) const {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    const ImVec2 screen = ImGui::GetIO().DisplaySize;
    const CelestialBody& focused = system.body(state.focusedBody);

    if (state.onGround) {
        drawGroundPanel(screen);
    }
    if (state.timeSinceFocusChange < kNameDisplayDuration) {
        drawBodyName(focused, state.timeSinceFocusChange, screen);
    }
    if (state.showInfoPanel) {
        drawInfoPanel(system, focused, screen);
    }
    if (state.timeScale > 1.0f) {
        drawTimeWarpPanel(system, state.timeScale, screen);
    }
    if (state.comparingSizes) {
        drawComparisonPanel(system, state.comparedPlanet, screen);
    }

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}
