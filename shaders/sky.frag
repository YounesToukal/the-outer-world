#version 330 core

// Samples an equirectangular star map: +y is north, longitude 0 lies along +x.

in vec3 vDirection;

uniform sampler2D uStarfieldTexture;

out vec4 fragColor;

const float PI = 3.14159265359;
const float BRIGHTNESS = 1.2;

void main()
{
    vec3 direction = normalize(vDirection);
    vec2 uv = vec2(0.5 + atan(direction.z, direction.x) / (2.0 * PI), 0.5 - asin(direction.y) / PI);

    // u jumps from 1 to 0 where the map wraps around. Taking the derivatives of a copy that wraps
    // on the opposite side avoids picking the smallest mipmap level along that seam.
    vec2 uvShifted = vec2(fract(uv.x + 0.5), uv.y);
    vec2 dx = dFdx(uv);
    vec2 dy = dFdy(uv);
    vec2 dxShifted = dFdx(uvShifted);
    vec2 dyShifted = dFdy(uvShifted);
    if (abs(dxShifted.x) < abs(dx.x)) dx.x = dxShifted.x;
    if (abs(dyShifted.x) < abs(dy.x)) dy.x = dyShifted.x;

    vec3 color = textureGrad(uStarfieldTexture, uv, dx, dy).rgb;
    fragColor = vec4(color * BRIGHTNESS, 1.0);
}
