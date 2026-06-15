#version 300 es
precision highp float;

/**
 * \file
 * \author Rudy Castan
 * \date 2025 Spring
 * \par CS250 Computer Graphics II
 * \copyright DigiPen Institute of Technology
 */

in vec2 vTextureCoordinates;

layout(location = 0) out vec4 fFragmentColor;

uniform sampler2D uTex2d;

void main()
{
    fFragmentColor = texture(uTex2d, vTextureCoordinates);
}
