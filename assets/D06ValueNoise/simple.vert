#version 300 es
precision highp float;

/**
 * \file
 * \author Sehoon Kim(sehoon.kim@digipen.edu)
 * \date 2026 Spring
 * \par CS250 Computer Graphics II
 * \copyright DigiPen Institute of Technology
 */

layout(location = 0) in vec3 aVertexPosition;
layout(location = 1) in vec3 aVertexNormal;
layout(location = 2) in vec2 aVertexTextureCoordinates;

uniform mat4 uProjection;
uniform vec2 uTileScale;

out vec2 vTexCoords;

void main()
{
    gl_Position = uProjection * vec4(aVertexPosition, 1.0);
    vTexCoords = aVertexTextureCoordinates * uTileScale;
}
