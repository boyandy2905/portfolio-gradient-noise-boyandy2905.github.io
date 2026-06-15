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

in vec3 vColor;
in vec2 vTextureCoordinates;

uniform sampler2D uTex2d;

uniform float uTime;
uniform int uFrameCount;
uniform vec2 uMousePosition;
uniform vec2 uResolution;

layout(location = 0) out vec4 fFragmentColor;

void main()
{
    vec4 texColor = texture(uTex2d, vTextureCoordinates);

    float basepulse = 0.5 + 0.5 * sin(uTime * 0.75);
    float pulse = smoothstep(0.70, 0.95, basepulse);

    vec3 textureColor = mix(vColor, texColor.rgb, 0.55);

    float brightness = 0.92 + 0.08 * sin(uTime * 1.2);
    vec3 gradientColor = vColor * brightness;

    vec3 finalColor = mix(textureColor, gradientColor, pulse);

    fFragmentColor = vec4(finalColor, texColor.a);
}
