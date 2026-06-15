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

uniform sampler2D uShadowMap;

void main()
{
    fFragmentColor = texture(uShadowMap, vTextureCoordinates);
}
