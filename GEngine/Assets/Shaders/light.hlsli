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
