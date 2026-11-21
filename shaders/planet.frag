#version 330 core

// Lambertian surface lit by the Sun, with a rim glow where the atmosphere is seen edge-on.

in vec3 vWorldPosition;
in vec3 vNormal;
in vec2 vTexCoord;

uniform sampler2D uSurfaceTexture;
uniform vec3 uLightPosition;
uniform vec3 uCameraPosition;
uniform vec3 uAtmosphereColor;  // black for bodies without an atmosphere

out vec4 fragColor;

const float AMBIENT = 0.05;
const float RIM_EXPONENT = 3.0;
const float RIM_STRENGTH = 0.15;

void main()
{
    vec3 albedo = texture(uSurfaceTexture, vTexCoord).rgb;
    vec3 normal = normalize(vNormal);
    vec3 lightDirection = normalize(uLightPosition - vWorldPosition);
    vec3 viewDirection = normalize(uCameraPosition - vWorldPosition);

    float diffuse = max(dot(normal, lightDirection), 0.0);
    float rim = pow(1.0 - max(dot(viewDirection, normal), 0.0), RIM_EXPONENT);

    vec3 color = albedo * (AMBIENT + diffuse) + uAtmosphereColor * rim * RIM_STRENGTH;
    fragColor = vec4(color, 1.0);
}
