// SRC Symbol Name: s_BSPPixelShader

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
    float4 col = baseTexture.Sample(texSampler, input.uv);

    // simple lambertian lighting: https://bentobaux.github.io/posts/basic-lighting-models-in-hlsl/
    const float3 lightDir = normalize(float3(0.4f, 1.f, 0.3f));
    const float Id = 0.65f * saturate(dot(input.normal, lightDir));

    return float4(Id * col.rgb * float3(1.f, 1.f, 1.f), 1.f);
}
