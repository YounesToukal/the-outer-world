#version 330 core

// Day and night surfaces blended across the terminator, a cloud layer, and atmospheric scattering
// approximated with view-dependent (Fresnel-like) terms.

in vec3 vWorldPosition;
in vec3 vNormal;
in vec2 vTexCoord;

uniform sampler2D uDayTexture;
uniform sampler2D uNightTexture;   // city lights
uniform sampler2D uCloudTexture;   // cloud cover in the alpha channel
uniform vec3 uLightPosition;
uniform vec3 uCameraPosition;
uniform vec3 uAtmosphereColor;     // Rayleigh scattering color

out vec4 fragColor;

const vec3 MIE_COLOR = vec3(1.0, 0.95, 0.85);
const vec3 NIGHT_RIM_COLOR = vec3(0.15, 0.2, 0.35);
const float ATMOSPHERE_THICKNESS = 0.3;
const float RAYLEIGH_STRENGTH = 4.5;
const float MIE_STRENGTH = 1.2;
const float FRESNEL_EXPONENT = 3.0;

const float CLOUD_OPACITY = 0.85;
const float CLOUD_SHADOW_STRENGTH = 0.3;

const float TERMINATOR_WIDTH = 0.15;
const float DAY_AMBIENT = 0.05;
const float NIGHT_AMBIENT = 0.05;
const float CITY_LIGHT_STRENGTH = 1.5;

void main()
{
    vec3 dayColor = texture(uDayTexture, vTexCoord).rgb;
    vec3 cityLights = texture(uNightTexture, vTexCoord).rgb;
    vec4 clouds = texture(uCloudTexture, vTexCoord);

    vec3 normal = normalize(vNormal);
    vec3 lightDirection = normalize(uLightPosition - vWorldPosition);
    vec3 viewDirection = normalize(uCameraPosition - vWorldPosition);

    // Surface: 0 on the night side, 1 on the day side, smooth across the terminator.
    float sunElevation = dot(normal, lightDirection);
    float daylight = smoothstep(-TERMINATOR_WIDTH, TERMINATOR_WIDTH, sunElevation);
    float diffuse = max(sunElevation, 0.0);

    vec3 litDay = dayColor * (diffuse + DAY_AMBIENT);
    vec3 litNight = cityLights * CITY_LIGHT_STRENGTH + dayColor * NIGHT_AMBIENT;
    vec3 surface = mix(litNight, litDay, daylight);

    // Clouds darken the ground below them on the day side.
    float cloudCover = clouds.a;
    surface *= mix(1.0, 1.0 - CLOUD_SHADOW_STRENGTH, cloudCover * daylight);
    vec3 litClouds = clouds.rgb * (diffuse * 1.2 + 0.1) * cloudCover * CLOUD_OPACITY;
    vec3 color = mix(surface, litClouds + surface * (1.0 - cloudCover * CLOUD_OPACITY), cloudCover);

    // Atmosphere: Rayleigh rim on the day side, forward Mie scattering toward the Sun.
    float facing = max(dot(viewDirection, normal), 0.0);
    float fresnel = pow(1.0 - facing, FRESNEL_EXPONENT);

    vec3 rayleigh = uAtmosphereColor * fresnel * RAYLEIGH_STRENGTH * daylight;
    float mieAlignment = pow(max(dot(lightDirection, viewDirection), 0.0), 8.0);
    vec3 mie = MIE_COLOR * mieAlignment * MIE_STRENGTH * fresnel * smoothstep(0.3, 0.7, daylight);
    vec3 scattering = (rayleigh + mie) * fresnel * ATMOSPHERE_THICKNESS;

    vec3 dayGlow = uAtmosphereColor * pow(1.0 - facing, 3.5) * 0.4 * daylight;
    vec3 nightGlow = NIGHT_RIM_COLOR * pow(1.0 - facing, 5.0) * 0.3 * (1.0 - daylight);
    color += scattering + dayGlow + nightGlow;

    // Seen from farther away, the haze along the rim grows stronger.
    float haze = smoothstep(0.5, 3.0, length(uCameraPosition - vWorldPosition));
    color += scattering * 0.5 * haze * fresnel * daylight;

    fragColor = vec4(color, 1.0);
}
