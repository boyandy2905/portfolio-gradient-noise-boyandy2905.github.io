#version 300 es
precision highp float;

/**
 * \file
 * \author Rudy Castan
 * \author Sehoon Kim(sehoon.kim@digipen.edu)
 * \date 2025 Spring
 * \par CS250 Computer Graphics II
 * \copyright DigiPen Institute of Technology
 */

layout(location = 0) in vec3 aVertexPosition;
layout(location = 1) in vec3 aVertexColor;
layout(location = 2) in vec2 aVertexTextureCoordinates;

uniform float uTime;
uniform int uFrameCount;
uniform vec2 uMousePosition;
uniform vec2 uResolution;

out vec3 vColor;
out vec2 vTextureCoordinates;

void main()
{
    vec3 pos = aVertexPosition;

    vec2 mouse01 = uMousePosition / uResolution;
    mouse01 = clamp(mouse01, vec2(0.0), vec2(1.0));

    vec2 mouseNDC = vec2(
        mouse01.x * 2.0 - 1.0,
        mouse01.y * 2.0 - 1.0
    );

    float rawPulse = 0.5 + 0.5 * sin(uTime * 1.1);
    float sizePulse = smoothstep(0.0, 1.0, rawPulse);

    const float PI = 3.14159265;

    float rotateDuration = 4.0;
    float pauseDuration  = 1.5;
    float cycleDuration  = rotateDuration + pauseDuration;

    float cycleTime = mod(uTime, cycleDuration);

    float rotateProgress = clamp(cycleTime / rotateDuration, 0.0, 1.0);

    rotateProgress = smoothstep(0.0, 1.0, rotateProgress);

    float angle = rotateProgress * 4.0 * PI;

    float c = cos(angle);
    float s = sin(angle);

    vec2 rotated;
    rotated.x = pos.x * c - pos.y * s;
    rotated.y = pos.x * s + pos.y * c;

    float wobbleStrength = mix(0.11, 0.02, sizePulse);

    float horizontalWobble = sin(uTime * 6.0 + pos.y * 8.0) * wobbleStrength;
    float verticalWobble   = cos(uTime * 5.0 + pos.x * 7.0) * wobbleStrength * 0.45;

    rotated.x += horizontalWobble * pos.y;
    rotated.y += verticalWobble * pos.x;

    float scale = mix(0.25, 1.25, sizePulse);

    float centerAmount = smoothstep(0.45, 1.0, sizePulse);
    vec2 followPosition = mix(mouseNDC, vec2(0.0, 0.0), centerAmount);

    pos.xy = rotated * scale + followPosition;

    gl_Position = vec4(pos, 1.0);
    vColor              = aVertexColor;
    vTextureCoordinates = aVertexTextureCoordinates;
}
