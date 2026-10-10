#ifndef SHADER_MATH_H
#define SHADER_MATH_H

static const float PI = 3.14159265358979323846264f;

float Luminance(float3 color) {
    return dot(color, float3(0.2126f, 0.7152f, 0.0722f));
}

#endif // SHADER_MATH_H
