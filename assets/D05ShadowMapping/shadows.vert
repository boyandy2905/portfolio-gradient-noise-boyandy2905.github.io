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
layout(location = 1) in vec3 aVertexNormal;

out vec3 vNormalInViewSpace;
out vec3 vPositionInViewSpace;
out vec4 vPositionInShadowSpace;

uniform mat4 uModelMatrix;
uniform mat4 uViewMatrix;
uniform mat4 uProjection;
uniform mat3 uNormalMatrix;
uniform mat4 uShadowMatrix;

void main()
{
    vec4 positionInWorldSpace = uModelMatrix * vec4(aVertexPosition, 1.0);
    vec4 positionInViewSpace  = uViewMatrix * positionInWorldSpace;

    gl_Position = uProjection * positionInViewSpace;

    vNormalInViewSpace     = normalize(uNormalMatrix * aVertexNormal);
    vPositionInViewSpace   = positionInViewSpace.xyz;
    vPositionInShadowSpace = uShadowMatrix * positionInWorldSpace;
}
