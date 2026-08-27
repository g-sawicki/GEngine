enum LightType {
    DIRECTIONAL_LIGHT,
    POINT_LIGHT,
    SPOT_LIGHT,
};

struct LightData {
    float3   position;
    uint32_t type;
    float3   direction;
    float3   color;
    float    intensity;
};

struct CascadedShadowMapsData {
    row_major float4x4 lightViewProjection[4];
    float4 cascadeSplits;
    float  shadowMapTexelSize;
    float  shadowBias;
    float  shadowSlopeScaleBias;
    float  normalOffsetScale;
    uint   shadowEnabled;
    uint   cascadeCount;
};

uint SelectCascade(CascadedShadowMapsData csmData, float viewDepth) {
    if (viewDepth <= csmData.cascadeSplits.x)
        return 0;
    if (viewDepth <= csmData.cascadeSplits.y)
        return 1;
    if (viewDepth <= csmData.cascadeSplits.z)
        return 2;
    return 3;
}

float SampleCascadeShadow(CascadedShadowMapsData csmData, SamplerComparisonState shadowSampler, float3 worldPos,
                          Texture2DArray shadowMap, uint cascade) {
    float4 positionLightSpace = mul(float4(worldPos, 1.0f), csmData.lightViewProjection[cascade]);
    float3 ndc = positionLightSpace.xyz / positionLightSpace.w;
    float2 uv = float2(ndc.x * 0.5f + 0.5f, 1.0f - (ndc.y * 0.5f + 0.5f));

    float shadow = 0.0;
    [unroll]
    for (int x = -1; x <= 1; ++x) {
        [unroll]
        for (int y = -1; y <= 1; ++y) {
            float2 offset = float2(x, y) * csmData.shadowMapTexelSize;
            shadow += shadowMap.SampleCmpLevelZero(shadowSampler, float3(uv + offset, cascade),
                                                   ndc.z - csmData.shadowBias);
        }
    }

    return shadow / 9.0f;
}

float ComputeShadow(CascadedShadowMapsData csmData, SamplerComparisonState shadowSampler, float3 cameraPosition,
                    float3 cameraForward, float3 worldPos, Texture2DArray shadowMap) {
    if (!csmData.shadowEnabled || csmData.cascadeCount == 0)
        return 1.0f;

    float viewDepth = dot(worldPos - cameraPosition, cameraForward);
    uint cascade = SelectCascade(csmData, viewDepth);

    float split = cascade == 0   ? csmData.cascadeSplits.x
                  : cascade == 1 ? csmData.cascadeSplits.y
                  : cascade == 2 ? csmData.cascadeSplits.z
                                 : 1e30f;
    float blendStart = split * 0.8f;
    float blend = saturate((viewDepth - blendStart) / max(split - blendStart, 1e-6f));

    float shadow = SampleCascadeShadow(csmData, shadowSampler, worldPos, shadowMap, cascade);
    if (cascade < csmData.cascadeCount - 1)
        shadow = lerp(shadow, SampleCascadeShadow(csmData, shadowSampler, worldPos, shadowMap, cascade + 1), blend);

    return shadow;
}

float3 CalculateBlinnPhong(LightData lightData, float3 lightDir, float3 viewDir, float3 worldNormal, float3 diffuseTex,
                           float3 specularTex) {
    float NdotL = saturate(dot(worldNormal, -lightDir));
    float3 diffuse = diffuseTex * NdotL;

    float3 halfDir = normalize(viewDir - lightDir);
    float specFactor = (NdotL > 0.0f) ? pow(saturate(dot(worldNormal, halfDir)), 64.0f) : 0.0f;
    float3 specular = specularTex * specFactor;

    float3 lightColor = lightData.color * lightData.intensity;
    return (diffuse + specular) * lightColor;
}

float3 CalculateDirectionalLight(LightData lightData, float3 viewDir, float3 worldNormal, float3 diffuseTex,
                                 float3 specularTex, float shadow) {
    float3 lightDir = normalize(lightData.direction);
    return CalculateBlinnPhong(lightData, lightDir, viewDir, worldNormal, diffuseTex, specularTex) * shadow;
}

float3 CalculatePointLight(LightData lightData, float3 worldPos, float3 viewDir, float3 worldNormal, float3 diffuseTex, float3 specularTex) {
    float distance = length(worldPos - lightData.position);
    float attenuation = 1.0 / (1.0f + 0.09f * distance + 0.032f * (distance * distance));

    float3 lightDir = normalize(worldPos - lightData.position);
    return CalculateBlinnPhong(lightData, lightDir, viewDir, worldNormal, diffuseTex, specularTex) * attenuation;
}

float3 CalculateLight(LightData lightData, float3 worldPos, float3 viewDir, float3 worldNormal, float3 diffuseTex, float3 specularTex,
                      float shadow) {
    switch(lightData.type) {
        case DIRECTIONAL_LIGHT:
            return CalculateDirectionalLight(lightData, viewDir, worldNormal, diffuseTex, specularTex, shadow);
        case POINT_LIGHT:
            return CalculatePointLight(lightData, worldPos, viewDir, worldNormal, diffuseTex, specularTex);
        case SPOT_LIGHT:
            break;
    }
    return 0.0f;
}

float3 CalculateDirectLighting(uint32_t lightIndex, uint32_t lightCount, float3 worldPos, float3 viewDir, float3 worldNormal,
                               float3 diffuseTex, float3 specularTex, float shadow) {
    StructuredBuffer<LightData> lightDataSB = ResourceDescriptorHeap[lightIndex];
    float3 result = 0.0f;
    for (uint32_t i = 0; i < lightCount; ++i) {
        LightData lightData = lightDataSB[i];
        result += CalculateLight(lightData, worldPos, viewDir, worldNormal, diffuseTex, specularTex, shadow);
    }
    return result;
}
