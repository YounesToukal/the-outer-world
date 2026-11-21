#version 330 core

// A cube around the camera. uView has no translation, so the sky stays infinitely far away.

layout(location = 0) in vec3 aPosition;

uniform mat4 uView;
uniform mat4 uProjection;

out vec3 vDirection;

void main()
{
    vDirection = aPosition;
    vec4 position = uProjection * uView * vec4(aPosition, 1.0);
    gl_Position = position.xyww;  // depth 1.0: behind everything else
}
