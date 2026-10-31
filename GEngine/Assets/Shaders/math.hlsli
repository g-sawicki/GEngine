#ifndef SHADER_MATH_H
#define SHADER_MATH_H

static const float PI = 3.14159265358979323846264f;

float Luminance(float3 color) {
    return dot(color, float3(0.2126f, 0.7152f, 0.0722f));
}

float3 DepthReprojection(float depth, float2 uv, row_major float4x4 inverseMatrix) {
    float3 ndc = float3(uv * 2.0f - 1.0f, depth);
    ndc.y = -ndc.y;
    float4 homogeneous = mul(float4(ndc, 1.0f), inverseMatrix);
    float3 position = homogeneous.xyz / homogeneous.w;
    return position;
}

float2 CenterBiasedUVFromPixelCoordinate(uint2 pixelCoord, uint2 resolution) {
    return (float2(pixelCoord) + 0.5f) / float2(resolution);
}

#endif // SHADER_MATH_H
