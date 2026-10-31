#include "cubemap_bake.hlsli"

[shader("pixel")]
float4 PSMain(PSInput input) : SV_TARGET {
    TextureCube<float3> environmentTexture = ResourceDescriptorHeap[constantsCB.InputIndex];

    const float3 normal = normalize(input.cubeDirection);

    const float3 up = abs(normal.z) < 0.999f ? float3(0.0f, 0.0f, 1.0f) : float3(1.0f, 0.0f, 0.0f);
    const float3 tangent = normalize(cross(up, normal));
    const float3 bitangent = cross(normal, tangent);

    const float sampleDelta = 0.025f;
    uint sampleCount = 0;
    float3 irradiance = float3(0.0f, 0.0f, 0.0f);
    [loop]
    for (float phi = 0.0f; phi < PI * 2.0f; phi += sampleDelta) {
        [loop]
        for (float theta = 0.0f; theta < PI * 0.5f; theta += sampleDelta) {
            const float3 tangentSample = float3(sin(theta) * cos(phi), sin(theta) * sin(phi), cos(theta));
            const float3 sampleVec = tangentSample.x * tangent + tangentSample.y * bitangent + tangentSample.z * normal;
            irradiance += environmentTexture.Sample(texSampler, sampleVec) * cos(theta) * sin(theta);
            ++sampleCount;
        }
    }
    irradiance = PI * irradiance * (1.0f / float(sampleCount));
    return float4(irradiance, 1.0f);
}
