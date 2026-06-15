#version 300 es
precision highp float;

/**
 * \file
 * \author Rudy Castan
 * \date 2025 Spring
 * \par CS250 Computer Graphics II
 * \copyright DigiPen Institute of Technology
 */

layout(location = 0) out vec4 fDepth;

uniform float     uNearDistance;
uniform float     uFarDistance;

/*
The LinearizeDepth function is used to linearize the depth value stored in the depth texture (ShadowMap). Depth values in a standard depth buffer are usually not linear and are more densely packed
closer to the camera. Linearizing the depth values makes the visualizatoin more perceptable.

https://www.geeks3d.com/hacklab/20200209/demo-depth-buffer-visualization/
*/
float LinearizeDepth(float depth)
{
    float zNear = uNearDistance;
    float zFar  = uFarDistance;
    return (2.0 * zNear) / (zFar + zNear - depth * (zFar - zNear));
}

void main()
{
    fDepth = vec4(vec3(LinearizeDepth(gl_FragCoord.z)),1.0f);
}
