// Generates the equirectangular sky map used as the background of the scene.
//
// The map has three layers:
//   1. emission nebulae (Orion, Carina, Cygnus): Gaussian clouds modulated by fractal noise,
//   2. every star of the Yale Bright Star Catalogue down to magnitude 6.5, at its real position,
//      with a brightness derived from its magnitude and a tint derived from its B-V color index,
//   3. fainter background stars scattered uniformly over the sphere.
//
// Layout: north at the top, right ascension 0h at the center and increasing to the left,
// which is how the sky appears from inside the celestial sphere.
//
// Usage: starfield-generator <catalog.csv> <output.png> [width]

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <exception>
#include <fstream>
#include <iostream>
#include <random>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

constexpr float kPi = 3.14159265358979f;
constexpr float kTwoPi = 2.0f * kPi;

constexpr float kFaintestMagnitude = 6.5f;
constexpr int kBackgroundStarCount = 10000;
constexpr std::uint32_t kSeed = 2026;

struct Color {
    float r, g, b;
};

struct Star {
    float rightAscension;  // radians, [0, 2pi)
    float declination;     // radians, [-pi/2, pi/2]
    float brightness;      // [0, 1]
    float size;            // splat radius in pixels
    Color color;
};

struct Nebula {
    float rightAscension;  // center, radians
    float declination;     // center, radians
    float spreadRa;        // Gaussian standard deviation along right ascension, radians
    float spreadDec;       // Gaussian standard deviation along declination, radians
    float intensity;
    float noiseScale;
    float contrast;        // exponent applied to the noise
};

constexpr Color kNebulaColor{0.75f, 0.50f, 1.00f};
constexpr Color kBlueWhite{0.65f, 0.80f, 1.00f};

float radians(float degrees) { return degrees * kPi / 180.0f; }

float uniform(std::mt19937& random) {
    // Uses the top 24 bits so the result is identical with every standard library.
    return static_cast<float>(random() >> 8) * (1.0f / 16777216.0f);
}

float uniform(std::mt19937& random, float low, float high) {
    return low + (high - low) * uniform(random);
}

// ---------------------------------------------------------------------------------------------
// Catalog

float parseField(const std::string& field, float fallback) {
    return field.empty() ? fallback : std::stof(field);
}

std::vector<std::string> splitCsvLine(const std::string& line) {
    std::vector<std::string> fields;
    std::stringstream stream(line);
    std::string field;
    while (std::getline(stream, field, ',')) {
        fields.push_back(field);
    }
    return fields;
}

// Maps the B-V color index to a tint. The palette stays close to white, as stars appear
// to the eye, with a cool tint for hot O and B stars and a warm tint for K and M stars.
Color colorFromColorIndex(float colorIndex) {
    if (colorIndex < -0.2f) return kBlueWhite;
    if (colorIndex < 0.3f) return {1.00f, 1.00f, 1.00f};
    if (colorIndex < 0.8f) return {1.00f, 0.98f, 0.95f};
    return {1.00f, 0.96f, 0.90f};
}

// Yale Bright Star Catalogue, 5th revised edition (CSV export of VizieR V/50).
std::vector<Star> loadCatalogStars(const std::string& path) {
    std::ifstream file(path);
    if (!file) {
        throw std::runtime_error("Cannot open " + path);
    }

    enum Column { RaHours = 19, RaMinutes, RaSeconds, DecSign, DecDegrees, DecMinutes, DecSeconds,
                  VisualMagnitude = 28, ColorIndex = 31 };

    struct Entry {
        float rightAscension, declination, magnitude, colorIndex;
    };
    std::vector<Entry> entries;

    std::string line;
    std::getline(file, line);  // header
    while (std::getline(file, line)) {
        const std::vector<std::string> fields = splitCsvLine(line);
        if (fields.size() <= ColorIndex || fields[VisualMagnitude].empty() || fields[RaHours].empty()) {
            continue;  // a few entries are novae or non-stellar objects without coordinates
        }

        const float magnitude = std::stof(fields[VisualMagnitude]);
        if (magnitude > kFaintestMagnitude) {
            continue;
        }

        const float hours = parseField(fields[RaHours], 0.0f) + parseField(fields[RaMinutes], 0.0f) / 60.0f +
                            parseField(fields[RaSeconds], 0.0f) / 3600.0f;
        float degrees = parseField(fields[DecDegrees], 0.0f) + parseField(fields[DecMinutes], 0.0f) / 60.0f +
                        parseField(fields[DecSeconds], 0.0f) / 3600.0f;
        if (fields[DecSign] == "-") {
            degrees = -degrees;
        }

        entries.push_back({hours / 24.0f * kTwoPi, radians(degrees), magnitude,
                           parseField(fields[ColorIndex], 0.0f)});
    }

    if (entries.empty()) {
        throw std::runtime_error("No stars read from " + path);
    }

    const auto [faintest, brightest] = std::minmax_element(
        entries.begin(), entries.end(), [](const Entry& a, const Entry& b) { return a.magnitude > b.magnitude; });

    std::vector<Star> stars;
    stars.reserve(entries.size());
    for (const Entry& entry : entries) {
        // Lower magnitude means brighter; the exponent below 1 lifts the brightest stars.
        const float linear = (faintest->magnitude - entry.magnitude) / (faintest->magnitude - brightest->magnitude);
        const float brightness = std::clamp(std::pow(linear, 0.7f), 0.05f, 1.0f);
        stars.push_back({entry.rightAscension, entry.declination, brightness, 1.0f + 3.0f * brightness,
                         colorFromColorIndex(entry.colorIndex)});
    }
    return stars;
}

// ---------------------------------------------------------------------------------------------
// Background stars

std::vector<Star> makeBackgroundStars(std::mt19937& random) {
    std::vector<Star> stars;
    stars.reserve(kBackgroundStarCount);
    for (int i = 0; i < kBackgroundStarCount; ++i) {
        const float rightAscension = uniform(random) * kTwoPi;
        const float declination = std::asin(uniform(random, -1.0f, 1.0f));  // uniform over the sphere

        const bool faint = uniform(random) < 0.3f;
        const float brightness = faint ? uniform(random, 0.03f, 0.15f) : uniform(random, 0.15f, 0.40f);

        const bool blue = uniform(random) < 0.05f;
        const float white = uniform(random, 0.95f, 1.0f);
        const Color color = blue ? kBlueWhite : Color{white, white, white};

        stars.push_back({rightAscension, declination, brightness, 1.0f, color});
    }
    return stars;
}

// ---------------------------------------------------------------------------------------------
// Nebulae

float latticeValue(int x, int y) {
    std::uint32_t n = static_cast<std::uint32_t>(x) + static_cast<std::uint32_t>(y) * 57u;
    n = (n << 13) ^ n;
    const std::uint32_t hashed = (n * (n * n * 15731u + 789221u) + 1376312589u) & 0x7fffffffu;
    return 1.0f - static_cast<float>(hashed) / 1073741824.0f;  // [-1, 1]
}

float cosineInterpolate(float a, float b, float t) {
    const float weight = (1.0f - std::cos(t * kPi)) * 0.5f;
    return a + (b - a) * weight;
}

float valueNoise(float x, float y) {
    const int ix = static_cast<int>(std::floor(x));
    const int iy = static_cast<int>(std::floor(y));
    const float fx = x - static_cast<float>(ix);
    const float fy = y - static_cast<float>(iy);

    const float bottom = cosineInterpolate(latticeValue(ix, iy), latticeValue(ix + 1, iy), fx);
    const float top = cosineInterpolate(latticeValue(ix, iy + 1), latticeValue(ix + 1, iy + 1), fx);
    return cosineInterpolate(bottom, top, fy);
}

// Fractional Brownian motion: five octaves, each at twice the frequency and half the amplitude.
float fractalNoise(float x, float y) {
    float sum = 0.0f;
    float amplitude = 1.0f;
    float frequency = 1.0f;
    float totalAmplitude = 0.0f;
    for (int octave = 0; octave < 5; ++octave) {
        sum += amplitude * valueNoise(x * frequency, y * frequency);
        totalAmplitude += amplitude;
        amplitude *= 0.5f;
        frequency *= 2.0f;
    }
    return sum / totalAmplitude;  // [-1, 1]
}

float wrapAngle(float angle) {
    // Into [-pi, pi]
    return angle - kTwoPi * std::round(angle / kTwoPi);
}

float nebulaDensity(const Nebula& nebula, float rightAscension, float declination) {
    const float dRa = wrapAngle(rightAscension - nebula.rightAscension);
    const float dDec = declination - nebula.declination;
    const float envelope = std::exp(-0.5f * (dRa * dRa) / (nebula.spreadRa * nebula.spreadRa)) *
                           std::exp(-0.5f * (dDec * dDec) / (nebula.spreadDec * nebula.spreadDec));
    if (envelope < 0.01f) {
        return 0.0f;
    }

    // Noise is sampled relative to the nebula center so it stays continuous across 0h.
    const float noise = fractalNoise((nebula.rightAscension + dRa) * nebula.noiseScale,
                                     (kPi / 2.0f - declination) * nebula.noiseScale);
    const float patchiness = std::pow((noise + 1.0f) * 0.5f, nebula.contrast);
    return std::clamp(envelope * patchiness, 0.0f, 1.0f);
}

const std::array<Nebula, 3> kNebulae{{
    {radians(85.0f), radians(-5.0f), radians(18.0f), radians(12.0f), 0.32f, 2.0f, 1.8f},    // Orion
    {radians(161.0f), radians(-59.0f), radians(22.5f), radians(18.0f), 0.32f, 1.8f, 2.0f},  // Carina
    {radians(312.0f), radians(44.0f), radians(27.0f), radians(15.0f), 0.36f, 2.2f, 1.8f},   // Cygnus
}};

// ---------------------------------------------------------------------------------------------
// Image

class SkyImage {
public:
    SkyImage(int width, int height)
        : width_(width), height_(height), pixels_(3u * static_cast<std::size_t>(width) * height, 0.0f) {}

    void drawNebulae() {
        for (int y = 0; y < height_; ++y) {
            const float declination = declinationAtRow(y);
            for (int x = 0; x < width_; ++x) {
                const float rightAscension = rightAscensionAtColumn(x);
                float density = 0.0f;
                for (const Nebula& nebula : kNebulae) {
                    density += nebula.intensity * nebulaDensity(nebula, rightAscension, declination);
                }
                add(x, y, kNebulaColor, density);
            }
        }
    }

    // Splats a star as a Gaussian point spread function.
    void drawStar(const Star& star) {
        const int centerX = columnAt(star.rightAscension);
        const int centerY = rowAt(star.declination);
        const int radius = std::clamp(static_cast<int>(star.size), 1, 4);
        const float sigma = 0.6f * static_cast<float>(radius);

        for (int dy = -radius; dy <= radius; ++dy) {
            const int y = centerY + dy;
            if (y < 0 || y >= height_) {
                continue;
            }
            for (int dx = -radius; dx <= radius; ++dx) {
                const int x = (centerX + dx + width_) % width_;
                const float distanceSquared = static_cast<float>(dx * dx + dy * dy);
                add(x, y, star.color, star.brightness * std::exp(-distanceSquared / (2.0f * sigma * sigma)));
            }
        }
    }

    void savePng(const std::string& path) const {
        std::vector<unsigned char> bytes(pixels_.size());
        std::transform(pixels_.begin(), pixels_.end(), bytes.begin(), [](float value) {
            return static_cast<unsigned char>(std::lround(std::clamp(value, 0.0f, 1.0f) * 255.0f));
        });
        if (!stbi_write_png(path.c_str(), width_, height_, 3, bytes.data(), width_ * 3)) {
            throw std::runtime_error("Cannot write " + path);
        }
    }

private:
    float rightAscensionAtColumn(int x) const {
        const float u = (static_cast<float>(x) + 0.5f) / static_cast<float>(width_);
        return std::fmod(kTwoPi * (1.5f - u), kTwoPi);
    }

    float declinationAtRow(int y) const {
        const float v = (static_cast<float>(y) + 0.5f) / static_cast<float>(height_);
        return kPi * (0.5f - v);
    }

    int columnAt(float rightAscension) const {
        const float u = std::fmod(1.5f - rightAscension / kTwoPi, 1.0f);
        return std::min(static_cast<int>(u * static_cast<float>(width_)), width_ - 1);
    }

    int rowAt(float declination) const {
        const float v = 0.5f - declination / kPi;
        return std::clamp(static_cast<int>(v * static_cast<float>(height_)), 0, height_ - 1);
    }

    void add(int x, int y, const Color& color, float amount) {
        float* pixel = &pixels_[3u * (static_cast<std::size_t>(y) * width_ + x)];
        pixel[0] += color.r * amount;
        pixel[1] += color.g * amount;
        pixel[2] += color.b * amount;
    }

    int width_;
    int height_;
    std::vector<float> pixels_;
};

}  // namespace

int main(int argc, char** argv) {
    if (argc < 3) {
        std::cerr << "Usage: " << argv[0] << " <catalog.csv> <output.png> [width]\n";
        return 1;
    }

    try {
        const int width = argc > 3 ? std::stoi(argv[3]) : 8192;
        if (width < 2 || width % 2 != 0) {
            throw std::invalid_argument("The width must be a positive even number");
        }

        std::mt19937 random(kSeed);
        const std::vector<Star> catalogStars = loadCatalogStars(argv[1]);
        const std::vector<Star> backgroundStars = makeBackgroundStars(random);

        SkyImage image(width, width / 2);
        image.drawNebulae();
        for (const Star& star : catalogStars) image.drawStar(star);
        for (const Star& star : backgroundStars) image.drawStar(star);
        image.savePng(argv[2]);

        std::cout << "Wrote " << argv[2] << " (" << width << "x" << width / 2 << ", " << catalogStars.size()
                  << " catalog stars, " << backgroundStars.size() << " background stars)\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "Error: " << error.what() << '\n';
        return 1;
    }
}
