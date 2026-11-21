#version 330 core

// Self-luminous surface with a slow brightness pulse and a corona that brightens toward the limb.

in vec3 vWorldPosition;
in vec3 vNormal;
in vec2 vTexCoord;

uniform sampler2D uSurfaceTexture;
uniform vec3 uCameraPosition;
uniform vec3 uAtmosphereColor;  // corona color
uniform float uTime;

out vec4 fragColor;

const float EMISSION = 2.5;
const float PULSE_AMPLITUDE = 0.05;
const float PULSE_FREQUENCY = 0.3;
const float CORONA_STRENGTH = 0.4;

void main()
{
    vec3 surface = texture(uSurfaceTexture, vTexCoord).rgb;
    float pulse = 1.0 + PULSE_AMPLITUDE * sin(uTime * PULSE_FREQUENCY);
    vec3 emission = surface * EMISSION * pulse;

    vec3 normal = normalize(vNormal);
    vec3 viewDirection = normalize(uCameraPosition - vWorldPosition);
    float limb = 1.0 - max(dot(viewDirection, normal), 0.0);
    vec3 corona = uAtmosphereColor * CORONA_STRENGTH * limb * limb;

    fragColor = vec4(emission + corona, 1.0);
}
