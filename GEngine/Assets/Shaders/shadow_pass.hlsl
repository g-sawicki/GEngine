#include "Interop/Common.h"
#include "Interop/Light.h"
#include "Interop/ShadowPass.h"

struct VSInput {
    float3 position : POSITION;
};

struct PSInput {
    float4 position : SV_POSITION;
};

ConstantBuffer<ShadowPassConstants> constantsCB : register(b0);
ConstantBuffer<CascadedShadowMapsData> cascadedShadowMapsDataCB : register(b1);
ConstantBuffer<ObjectData> objectDataCB : register(b2);

[shader("vertex")]
PSInput VSMain(VSInput input) {
    PSInput output;
    const float4 worldPos = mul(float4(input.position, 1.0f), objectDataCB.world);
    const row_major float4x4 lightViewProjection = cascadedShadowMapsDataCB.lightViewProjection[constantsCB.CascadeIndex];
    output.position = mul(worldPos, lightViewProjection);
    return output;
}

[shader("pixel")]
void PSMain(PSInput input) {}
