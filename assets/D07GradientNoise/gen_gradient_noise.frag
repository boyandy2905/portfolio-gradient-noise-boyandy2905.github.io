#version 300 es
precision highp float;
precision highp int;

/**
 * \file
 * \author Sehoon Kim(sehoon.kim@digipen.edu)
 * \date 2026 Spring
 * \par CS250 Computer Graphics II
 * \copyright DigiPen Institute of Technology
 */

in vec2 vTextureCoordinates;

layout(location = 0) out float fFragmentColor;

uniform float uZ;
uniform int uPattern;

const int PERIOD = 64;
const float PERIOD_FLOAT = 64.0;

const int permutation[128] = int[128](
    // size 64
    27, 33, 14, 52, 24, 36, 46, 40, 26, 7, 49, 57, 59, 2, 42, 61,
    9, 3, 12, 63, 37, 53, 17, 8, 44, 35, 30, 22, 6, 18, 60, 55,
    31, 13, 21, 5, 47, 25, 38, 28, 32, 45, 43, 48, 23, 58, 62, 41,
    11, 29, 34, 54, 0, 1, 20, 19, 16, 4, 15, 50, 10, 39, 56, 51,

    // repeat
    27, 33, 14, 52, 24, 36, 46, 40, 26, 7, 49, 57, 59, 2, 42, 61,
    9, 3, 12, 63, 37, 53, 17, 8, 44, 35, 30, 22, 6, 18, 60, 55,
    31, 13, 21, 5, 47, 25, 38, 28, 32, 45, 43, 48, 23, 58, 62, 41,
    11, 29, 34, 54, 0, 1, 20, 19, 16, 4, 15, 50, 10, 39, 56, 51
);

int hash3(int x, int y, int z)
{
    x = x & 63;
    y = y & 63;
    z = z & 63;

    return permutation[permutation[permutation[x] + y] + z];
}

vec3 gradient_direction(int hash_value)
{
    int h = hash_value % 12;

    if (h == 0)  return vec3( 1.0,  1.0,  0.0);
    if (h == 1)  return vec3(-1.0,  1.0,  0.0);
    if (h == 2)  return vec3( 1.0, -1.0,  0.0);
    if (h == 3)  return vec3(-1.0, -1.0,  0.0);

    if (h == 4)  return vec3( 1.0,  0.0,  1.0);
    if (h == 5)  return vec3(-1.0,  0.0,  1.0);
    if (h == 6)  return vec3( 1.0,  0.0, -1.0);
    if (h == 7)  return vec3(-1.0,  0.0, -1.0);

    if (h == 8)  return vec3( 0.0,  1.0,  1.0);
    if (h == 9)  return vec3( 0.0, -1.0,  1.0);
    if (h == 10) return vec3( 0.0,  1.0, -1.0);

    return vec3(0.0, -1.0, -1.0);
}

float fade(float t)
{
    return t * t * t * (t * (t * 6.0 - 15.0) + 10.0);
}

vec3 fade(vec3 t)
{
    return vec3(fade(t.x), fade(t.y), fade(t.z));
}

float gradient_dot(int ix, int iy, int iz, vec3 distance_vector)
{
    vec3 gradient = gradient_direction(hash3(ix, iy, iz));
    return dot(gradient, distance_vector);
}

float gradient_noise(vec3 position)
{
    ivec3 base = ivec3(floor(position));
    vec3 local = fract(position);

    ivec3 next = base + ivec3(1);

    float n000 = gradient_dot(base.x, base.y, base.z, local - vec3(0.0, 0.0, 0.0));
    float n100 = gradient_dot(next.x, base.y, base.z, local - vec3(1.0, 0.0, 0.0));
    float n010 = gradient_dot(base.x, next.y, base.z, local - vec3(0.0, 1.0, 0.0));
    float n110 = gradient_dot(next.x, next.y, base.z, local - vec3(1.0, 1.0, 0.0));

    float n001 = gradient_dot(base.x, base.y, next.z, local - vec3(0.0, 0.0, 1.0));
    float n101 = gradient_dot(next.x, base.y, next.z, local - vec3(1.0, 0.0, 1.0));
    float n011 = gradient_dot(base.x, next.y, next.z, local - vec3(0.0, 1.0, 1.0));
    float n111 = gradient_dot(next.x, next.y, next.z, local - vec3(1.0, 1.0, 1.0));

    vec3 u = fade(local);

    float nx00 = mix(n000, n100, u.x);
    float nx10 = mix(n010, n110, u.x);
    float nx01 = mix(n001, n101, u.x);
    float nx11 = mix(n011, n111, u.x);

    float nxy0 = mix(nx00, nx10, u.y);
    float nxy1 = mix(nx01, nx11, u.y);

    return mix(nxy0, nxy1, u.z);
}

float normalize_noise(float value)
{
    return clamp(value * 0.5 + 0.5, 0.0, 1.0);
}

float gradient_noise_01(vec3 position)
{
    return normalize_noise(gradient_noise(position));
}

float fractal_sum(vec3 position)
{
    float value = 0.0;
    float amplitude = 0.5;

    for (int octave = 0; octave < 5; ++octave)
    {
        value += amplitude * gradient_noise_01(position);
        position *= 2.0;
        amplitude *= 0.5;
    }

    return clamp(value, 0.0, 1.0);
}

float turbulence(vec3 position)
{
    float value = 0.0;
    float amplitude = 0.5;

    for (int octave = 0; octave < 5; ++octave)
    {
        value += amplitude * abs(gradient_noise(position));
        position *= 2.0;
        amplitude *= 0.5;
    }

    return clamp(value, 0.0, 1.0);
}

float marble(vec3 position)
{
    const float stripe_count = 20.0;

    float stripes = position.x / PERIOD_FLOAT * stripe_count;
    float distortion = 0.5 * fractal_sum(position);

    return 0.5 + 0.5 * sin((stripes + distortion) * 6.28318530718);
}

float wood(vec3 position)
{
    return fract(10.0 * gradient_noise_01(position));
}

void main()
{
    vec3 position = vec3(vTextureCoordinates * PERIOD_FLOAT, uZ);

    float value = 0.0;

    if (uPattern == 0)
    {
        value = gradient_noise_01(position);
    }
    else if (uPattern == 1)
    {
        value = fractal_sum(position);
    }
    else if (uPattern == 2)
    {
        value = turbulence(position);
    }
    else if (uPattern == 3)
    {
        value = marble(position);
    }
    else
    {
        value = wood(position);
    }

    fFragmentColor = clamp(value, 0.0, 1.0);
}
