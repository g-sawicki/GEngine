struct VSInput
{
    float4 position : POSITION;
    float4 color : COLOR;
};

struct PSInput
{
    float4 position : SV_POSITION;
    float4 color : COLOR;
};

cbuffer CameraConstants : register(b0)
{
    float4x4 ViewProjection;
};

PSInput VSMain(VSInput input)
{
    PSInput output;
    output.position = mul(input.position, ViewProjection);
    output.color = input.color;
    return output;
}

float4 PSMain(PSInput input) : SV_TARGET
{
    return input.color;
}
