#version 300 es
precision highp float;

/**
 * \file
 * \author Sehoon Kim(sehoon.kim@digipen.edu)
 * \date 2026 Spring
 * \par CS250 Computer Graphics II
 * \copyright DigiPen Institute of Technology
 */

in vec2 vTextureCoordinates;

layout(location = 0) out vec4 fFragmentColor;

uniform sampler2D uTex2d;

void main()
{
    float noise = texture(uTex2d, vTextureCoordinates).r;

    fFragmentColor = vec4(vec3(noise), 1.0);
}
