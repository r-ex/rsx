struct VS_Output
{
    float4 position : SV_POSITION;
    float3 worldPosition : POSITION;
    float4 color : COLOR;
    float3 normal : NORMAL;
    float2 uv : TEXCOORD;
};

Texture2D baseTexture : register(t0);
SamplerState texSampler : register(s0);

float4 ps_main(VS_Output input) : SV_Target
{
    // return input.color;
    return baseTexture.Sample(texSampler, input.uv);
}