#version 330 core

// One rock per instance: the instance transform places it in the ring plane,
// and uModel places the ring plane around Saturn.

layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aNormal;
layout(location = 3) in mat4 aInstanceTransform;  // locations 3 to 6
layout(location = 7) in vec3 aInstanceColor;

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProjection;

out vec3 vWorldPosition;
out vec3 vNormal;
out vec3 vColor;

void main()
{
    mat4 model = uModel * aInstanceTransform;
    vec4 worldPosition = model * vec4(aPosition, 1.0);
    vWorldPosition = worldPosition.xyz;
    vNormal = mat3(model) * aNormal;  // valid because the transforms only rotate and scale uniformly
    vColor = aInstanceColor;
    gl_Position = uProjection * uView * worldPosition;
}
