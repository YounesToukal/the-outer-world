#version 330 core

in vec3 vWorldPosition;
in vec3 vNormal;
in vec3 vColor;

uniform vec3 uLightPosition;

out vec4 fragColor;

const float AMBIENT = 0.05;

void main()
{
    vec3 normal = normalize(vNormal);
    vec3 lightDirection = normalize(uLightPosition - vWorldPosition);
    float diffuse = max(dot(normal, lightDirection), 0.0);
    fragColor = vec4(vColor * (AMBIENT + diffuse), 1.0);
}
