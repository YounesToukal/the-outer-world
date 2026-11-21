#include "overlay.h"

#include <algorithm>
#include <cmath>
#include <string>

#include <glm/gtc/constants.hpp>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

namespace {

constexpr float kNameDisplayDuration = 3.0f;  // seconds the name of a newly focused body stays visible
constexpr float kNameFadeDuration = 0.9f;
constexpr float kMargin = 10.0f;

const ImVec4 kTitleColor(0.4f, 0.8f, 1.0f, 1.0f);
const ImVec4 kModeColor(1.0f, 0.7f, 0.2f, 1.0f);
const ImVec4 kGroundColor(0.3f, 1.0f, 0.5f, 1.0f);
const ImVec4 kLabelColor(1.0f, 0.85f, 0.3f, 1.0f);
const ImVec4 kValueColor(1.0f, 1.0f, 0.5f, 1.0f);
const ImVec4 kHeadingColor(0.5f, 1.0f, 0.7f, 1.0f);
const ImVec4 kHintColor(0.7f, 0.7f, 0.7f, 1.0f);

constexpr ImGuiWindowFlags kPanelFlags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoInputs |
                                         ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoSavedSettings |
                                         ImGuiWindowFlags_NoFocusOnAppearing;

// The pivot selects the point of the panel placed at the position: (0, 0) top left, (1, 1) bottom right.
// A width of 0 sizes the panel to its content.
void beginPanel(const char* id, ImVec2 position, ImVec2 pivot, float width = 0.0f) {
    ImGui::SetNextWindowPos(position, ImGuiCond_Always, pivot);
    ImGui::SetNextWindowSize(ImVec2(width, 0.0f), ImGuiCond_Always);
    ImGui::Begin(id, nullptr, width > 0.0f ? kPanelFlags : kPanelFlags | ImGuiWindowFlags_AlwaysAutoResize);
}

void scaledText(const ImVec4& color, float scale, const char* text) {
    ImGui::PushFont(nullptr, ImGui::GetStyle().FontSizeBase * scale);
    ImGui::TextColored(color, "%s", text);
    ImGui::PopFont();
}

std::string withThousandsSeparators(long value) {
    std::string digits = std::to_string(value);
    for (int position = static_cast<int>(digits.size()) - 3; position > 0; position -= 3) {
        digits.insert(static_cast<std::size_t>(position), ",");
    }
    return digits;
}

void separator() {
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();
}

void drawBodyName(const CelestialBody& body, float timeSinceFocusChange, ImVec2 screen) {
    const float opacity = std::clamp((kNameDisplayDuration - timeSinceFocusChange) / kNameFadeDuration, 0.0f, 1.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, opacity);
    beginPanel("BodyName", ImVec2(screen.x * 0.5f, screen.y * 0.85f), ImVec2(0.5f, 0.5f));
    scaledText(kTitleColor, 2.5f, body.name.c_str());
    ImGui::End();
    ImGui::PopStyleVar();
}

void drawInfoPanel(const SolarSystem& system, const CelestialBody& body, ImVec2 screen) {
    beginPanel("Info", ImVec2(screen.x - kMargin, screen.y - kMargin), ImVec2(1.0f, 1.0f), 400.0f);

    scaledText(kTitleColor, 1.8f, body.name.c_str());
    separator();

    const std::string radius = withThousandsSeparators(std::lround(body.radiusKm)) + " km";
    ImGui::TextColored(kLabelColor, "Radius:");
    ImGui::SameLine();
    if (body.id == BodyId::Earth) {
        ImGui::Text("%s", radius.c_str());
    } else {
        ImGui::Text("%s, %.2f times Earth's", radius.c_str(), system.radiusInEarthRadii(body.id));
    }
    separator();

    ImGui::TextColored(kHeadingColor, "Facts");
    ImGui::Spacing();
    for (const std::string& fact : body.facts) {
        ImGui::BulletText("%s", fact.c_str());
    }

    ImGui::End();
}

void drawTimePanel(const SolarSystem& system, float timeScale, ImVec2 screen) {
    beginPanel("Time", ImVec2(screen.x - kMargin, kMargin), ImVec2(1.0f, 0.0f), 260.0f);

    ImGui::PushFont(nullptr, ImGui::GetStyle().FontSizeBase * 1.3f);
    ImGui::TextColored(kModeColor, "TIME x%.0f", timeScale);
    ImGui::PopFont();
    separator();

    // One Earth day is one turn of Earth around its axis.
    const float daysPerSecond = system.body(BodyId::Earth).spinSpeed * timeScale / glm::two_pi<float>();
    const float hoursPerSecond = daysPerSecond * 24.0f;
    ImGui::PushFont(nullptr, ImGui::GetStyle().FontSizeBase * 1.8f);
    if (daysPerSecond >= 1.0f) {
        ImGui::TextColored(kValueColor, "%.1f days/s", daysPerSecond);
    } else if (hoursPerSecond >= 1.0f) {
        ImGui::TextColored(kValueColor, "%.1f hours/s", hoursPerSecond);
    } else {
        ImGui::TextColored(kValueColor, "%.1f minutes/s", hoursPerSecond * 60.0f);
    }
    ImGui::PopFont();
    separator();

    ImGui::TextColored(kHintColor, "Up: faster   Down: slower");
    ImGui::End();
}

void drawGroundPanel(ImVec2 screen) {
    beginPanel("Ground", ImVec2(screen.x * 0.5f, 50.0f), ImVec2(0.5f, 0.0f));
    scaledText(kGroundColor, 2.0f, "GROUND VIEW");
    ImGui::Spacing();
    ImGui::TextColored(kHintColor, "Move the mouse to look around");
    ImGui::TextColored(kHintColor, "Press X to return to orbit");
    ImGui::End();
}

void drawComparisonPanel(const SolarSystem& system, BodyId compared, ImVec2 screen) {
    beginPanel("Comparison", ImVec2(screen.x * 0.5f, screen.y - kMargin), ImVec2(0.5f, 1.0f), 500.0f);

    scaledText(kModeColor, 1.5f, "SIZE COMPARISON");
    separator();

    const std::string& name = system.body(compared).name;
    ImGui::TextColored(kTitleColor, "Sun and %s", name.c_str());
    ImGui::Spacing();
    ImGui::TextColored(kValueColor, "The Sun's radius is %.1f times %s's",
                       system.radiusInEarthRadii(BodyId::Sun) / system.radiusInEarthRadii(compared), name.c_str());
    separator();

    ImGui::TextColored(kHintColor, "Left / Right: change body   C: leave comparison");
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
    if (state.comparingSizes) {
        drawComparisonPanel(system, state.comparedBody, screen);
    } else {
        const CelestialBody& focused = system.body(state.focusedBody);
        if (state.timeSinceFocusChange < kNameDisplayDuration) {
            drawBodyName(focused, state.timeSinceFocusChange, screen);
        }
        if (state.showInfoPanel) {
            drawInfoPanel(system, focused, screen);
        }
        if (state.onGround) {
            drawGroundPanel(screen);
        }
    }
    if (state.timeScale > 1.0f) {
        drawTimePanel(system, state.timeScale, screen);
    }

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}
