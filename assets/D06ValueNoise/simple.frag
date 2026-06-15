#version 300 es
precision highp float;

/**
 * \file
 * \author Sehoon Kim(sehoon.kim@digipen.edu)
 * \date 2026 Spring
 * \par CS250 Computer Graphics II
 * \copyright DigiPen Institute of Technology
 */

in vec2 vTexCoords;

layout(location = 0) out vec4 fFragmentColor;

uniform sampler2D uTex2d;

void main()
{
    vec4 color = texture(uTex2d, vTexCoords);
    fFragmentColor = vec4(color.rgb, 1.0);
}
