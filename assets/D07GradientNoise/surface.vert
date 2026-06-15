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
uniform mat4 uViewMatrix;
uniform mat4 uModelMatrix;
uniform float uTileScale;

uniform sampler2D uTex2d;

out vec2 vTextureCoordinates;

void main()
{
    vTextureCoordinates = aVertexTextureCoordinates * uTileScale;

    float noise = texture(uTex2d, vTextureCoordinates).r;

    vec3 displacedPosition = aVertexPosition;

    // Move the plane along its local z axis.
    // The C++ model matrix scales this z value with heightScale,
    // then rotates the plane into the final surface orientation.
    displacedPosition.z += noise;

    gl_Position = uProjection * uViewMatrix * uModelMatrix * vec4(displacedPosition, 1.0);
}
