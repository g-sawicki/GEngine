#include "cubemap_bake.hlsli"

float2 DirectionToEquirectUV(float3 direction) {
    float3 d = normalize(direction);
    // horizontal angle in [-pi, pi]
    float theta = atan2(d.z, d.x);
    // vertical angle in [-pi/2, pi/2]
    float phi = asin(clamp(d.y, -1.0f, 1.0f));
    return float2(0.5f + theta / (2.0f * PI), 0.5f - phi / PI);
}

[shader("pixel")]
float4 PSMain(PSInput input) : SV_TARGET {
    Texture2D<float4> inputTexture = ResourceDescriptorHeap[constantsCB.inputIndex];

    float3 direction = normalize(input.worldPosition);
    float2 uv = DirectionToEquirectUV(direction);
    float3 color = inputTexture.SampleLevel(texSampler, uv, 0.0f).rgb;
    return float4(color, 1.0f);
}
