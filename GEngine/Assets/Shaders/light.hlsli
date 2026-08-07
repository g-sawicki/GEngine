struct DirectionalLight {
    float3 direction;
    float  intensity;
    float3 color;
};

struct PointLight {
    float3 position;
    float  intensity;
    float3 color;
    float  attenuationConstant;
    float  attenuationLinear;
    float  attenuationQuadratic;
};

struct LightData
{
    row_major float4x4 lightViewProjection;
    float shadowMapTexelSize;
    float shadowBias;
    float shadowSlopeScaleBias;
    float normalOffsetScale;
    uint  shadowEnabled;
};
