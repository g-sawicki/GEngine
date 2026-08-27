#include "common.hlsli"

struct VSInput {
    float4 position : POSITION;
};

struct PSInput {
    float4 position : SV_POSITION;
};

struct RootConstants {
    uint cascadeIndex;
};

ConstantBuffer<CascadedShadowMapsData> cascadedShadowMapsDataCB : register(b0);
ConstantBuffer<ObjectConstants> objectConstantsCB : register(b1);
ConstantBuffer<RootConstants> constantsCB : register(b2);

[shader("vertex")]
PSInput VSMain(VSInput input) {
    PSInput output;
    float4 worldPos = mul(input.position, objectConstantsCB.world);
    output.position = mul(worldPos, cascadedShadowMapsDataCB.lightViewProjection[constantsCB.cascadeIndex]);
    return output;
}

[shader("pixel")]
void PSMain(PSInput input){}
