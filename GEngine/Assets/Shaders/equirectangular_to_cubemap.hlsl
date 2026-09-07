

struct VSInput {
    float3 position : POSITION;
};

struct PSInput {
    float4 position : SV_POSITION;
    float3 worldPosition : TEXCOORD0;
};

struct CameraData {
    row_major float4x4 viewProjection[6];
};

struct RootConstants {
    uint32_t InputIndex;
    uint32_t cameraIndex;
};

ConstantBuffer<CameraData> cameraDataCB : register(b0);
ConstantBuffer<RootConstants> constantsCB : register(b1);
SamplerState texSampler : register(s0);

PSInput VSMain(VSInput input) {
    PSInput output;
    output.position = mul(float4(input.position, 1.0), cameraDataCB.viewProjection[constantsCB.cameraIndex]);
    output.worldPosition = input.position;
    return output;
}

float2 DirectionToEquirectUV(float3 direction) {
    const float kPi = 3.14159265358979323846;
    float3 d = normalize(direction);
    // horizontal angle in [-pi, pi]
    float theta = atan2(d.z, d.x);
    // vertical angle in [-pi/2, pi/2]
    float phi = asin(clamp(d.y, -1.0f, 1.0f));
    return float2(0.5f + theta / (2.0f * kPi), 0.5f - phi / kPi);
}

float4 PSMain(PSInput input) : SV_TARGET {
    Texture2D<float4> inputTexture = ResourceDescriptorHeap[constantsCB.InputIndex];

    float3 direction = normalize(input.worldPosition);
    float2 uv = DirectionToEquirectUV(direction);
    float3 color = inputTexture.SampleLevel(texSampler, uv, 0.0f).rgb;
    return float4(color, 1.0f);
}