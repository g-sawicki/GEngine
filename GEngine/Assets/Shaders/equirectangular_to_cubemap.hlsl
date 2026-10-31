#include "cubemap_bake.hlsli"

float2 DirectionToEquirectUV(float3 direction) {
    // horizontal angle in [-pi, pi]
    const float theta = atan2(direction.z, direction.x);
    // vertical angle in [-pi/2, pi/2]
    const float phi = asin(clamp(direction.y, -1.0f, 1.0f));
    return float2(0.5f + theta / (2.0f * PI), 0.5f - phi / PI);
}

[shader("pixel")]
float4 PSMain(PSInput input) : SV_TARGET {
    Texture2D<float3> inputTexture = ResourceDescriptorHeap[constantsCB.InputIndex];

    const float3 direction = normalize(input.cubeDirection);
    const float2 uv = DirectionToEquirectUV(direction);
    const float3 color = inputTexture.Sample(equirectSampler, uv);
    return float4(color, 1.0f);
}
