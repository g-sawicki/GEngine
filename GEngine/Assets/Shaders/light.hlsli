#define PI 3.14159265358979323846264f

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
    float    cosInnerCone;
    float    cosOuterCone;
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

struct Material {
    float4 albedo;
    float  roughness;
    float  metallic;
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

float DistributionTrowbridgeReitzGGX(float3 normal, float3 halfDir, float roughness) {
    float alpha = roughness * roughness;
    float alpha2 = alpha * alpha;
    float NdotH = max(dot(normal, halfDir), 0.0f);
    float NdotH2 = NdotH * NdotH;
    float denominator = NdotH2 * (alpha2 - 1.0f) + 1.0f;
    denominator = PI * denominator * denominator;
    return alpha2 / denominator;
}

float kDirect(float roughness) {
    return (roughness + 1.0f) * (roughness + 1.0f) / 8.0f;
}

float kIBL(float roughness) {
    return roughness * roughness / 2.0f;
}

float GeometrySchlickGGX(float NdotV, float k) {
    return NdotV / (NdotV * (1.0f - k) + k);
}

float GeometrySmith(float3 normal, float3 viewDir, float3 lightDir, float k) {
    float NdotV = max(dot(normal, viewDir), 0.0f);
    float NdotL = max(dot(normal, lightDir), 0.0f);
    float ggx1 = GeometrySchlickGGX(NdotV, k);
    float ggx2 = GeometrySchlickGGX(NdotL, k);
    return ggx1 * ggx2;
}

float3 FresnelSchlick(float cosTheta, float3 F0) {
    return F0 + (1.0f - F0) * pow(1.0f - cosTheta, 5.0f);
}

struct CookTorranceResult {
    float3 specular;
    float3 kD;
};

CookTorranceResult CookTorranceBRDF(float3 lightDir, float3 viewDir, float3 normal, Material material) {
    float3 L = -lightDir;
    float3 halfDir = normalize(viewDir + L);
    float k = kDirect(material.roughness);
    float3 F0 = 0.04f;
    F0 = lerp(F0, material.albedo.xyz, material.metallic);

    float NdotL = max(dot(normal, L), 0.0f);
    float NdotV = max(dot(normal, viewDir), 0.0f);
    float cosTheta = max(dot(viewDir, halfDir), 0.0f);

    float  D = DistributionTrowbridgeReitzGGX(normal, halfDir, material.roughness);
    float3 F = FresnelSchlick(cosTheta, F0);
    float  G = GeometrySmith(normal, viewDir, L, k);
    float divisor = 4.0f * NdotL * NdotV + 0.0001f;

    CookTorranceResult result;
    result.specular = D * F * G / divisor;
    result.kD = (1.0f - F) * (1.0f - material.metallic);
    return result;
}

float3 BlinnPhong(LightData lightData, float3 lightDir, float3 viewDir, float3 normal, Material material) {
    float3 diffuse = material.albedo.xyz;
    float3 specular = lerp(0.04f, material.albedo.xyz, material.metallic);
    float shininess = max(pow(1.0f - material.roughness, 2.0f) * 256.0f, 1.0f);

    float NdotL = saturate(dot(normal, -lightDir));
    float3 halfDir = normalize(viewDir - lightDir);
    float specFactor = (NdotL > 0.0f) ? pow(saturate(dot(normal, halfDir)), shininess) : 0.0f;

    float3 lightColor = lightData.color * lightData.intensity;
    return (diffuse + specular * specFactor) * lightColor * NdotL;
}

float CalculateAttentuation(float distance) {
    return 1.0 / (distance * distance);
}

float3 CalculateDirectionalLight(LightData lightData, float3 viewDir, float3 normal, Material material, float shadow) {
    float3 lightDir = normalize(lightData.direction);
    float NdotL = saturate(dot(normal, -lightDir));
    float3 radiance = lightData.color * lightData.intensity;

    CookTorranceResult brdf = CookTorranceBRDF(lightDir, viewDir, normal, material);
    float3 diffuse = material.albedo.xyz / PI * brdf.kD;
    float3 specular = brdf.specular;

    return (diffuse + specular) * NdotL * radiance * shadow;
}

float3 CalculatePointLight(LightData lightData, float3 lightDir, float distance, float3 viewDir, float3 normal, Material material) {
    float attenuation = CalculateAttentuation(distance);
    float NdotL = saturate(dot(normal, -lightDir));
    float3 radiance = lightData.color * lightData.intensity * attenuation;

    CookTorranceResult brdf = CookTorranceBRDF(lightDir, viewDir, normal, material);
    float3 diffuse = material.albedo.xyz / PI * brdf.kD;
    float3 specular = brdf.specular;

    return (diffuse + specular) * NdotL * radiance;
}

float3 CalculateLight(LightData lightData, float3 worldPos, float3 viewDir, float3 normal, Material material, float shadow) {
    if (lightData.type == DIRECTIONAL_LIGHT)
        return CalculateDirectionalLight(lightData, viewDir, normal, material, shadow);

    float3 lightToWorldPos = worldPos - lightData.position;
    float3 worldPosDir = normalize(lightToWorldPos);
    float distance = length(lightToWorldPos);
    float3 intensity = CalculatePointLight(lightData, worldPosDir, distance, viewDir, normal, material);

    if (lightData.type == SPOT_LIGHT) {
        float3 lightDir = normalize(lightData.direction);
        float theta = dot(lightDir, worldPosDir);
        float epsilon = lightData.cosInnerCone - lightData.cosOuterCone;
        intensity *= saturate((theta - lightData.cosOuterCone) / epsilon);
    }

    return intensity;
}

float3 CalculateDirectLighting(uint32_t lightIndex, uint32_t lightCount, float3 worldPos, float3 viewDir, float3 normal,
                               Material material, float shadow) {
    StructuredBuffer<LightData> lightDataSB = ResourceDescriptorHeap[lightIndex];
    float3 result = 0.0f;
    for (uint32_t i = 0; i < lightCount; ++i) {
        LightData lightData = lightDataSB[i];
        result += CalculateLight(lightData, worldPos, viewDir, normal, material, shadow);
    }
    return result;
}
