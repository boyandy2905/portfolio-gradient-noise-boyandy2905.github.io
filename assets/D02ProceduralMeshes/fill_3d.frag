#version 300 es
precision highp float;

/**
 * \file
 * \author Rudy Castan
 * \date 2025 Spring
 * \par CS250 Computer Graphics II
 * \copyright DigiPen Institute of Technology
 */

layout(location = 0) out vec4 fFragmentColor;

uniform vec3 uDiffuseMaterial;

void main()
{
    fFragmentColor = vec4(uDiffuseMaterial, 1.0);
}
