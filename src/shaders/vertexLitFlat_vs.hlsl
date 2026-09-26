// SRC Symbol Name: s_VertexLitFlatShader

struct VS_Input
{
    float3 position : POSITION;
    float3 normal : NORMAL;
    float2 uv : TEXCOORD;    
    uint unk : UNK;
};

struct VS_Output
{
    float4 position : SV_POSITION;
    float3 worldPosition : POSITION;
    float4 color : COLOR;
    float3 normal : NORMAL;
    float2 uv : TEXCOORD;
};

cbuffer VS_TransformConstants : register(b0)
{
    float4x4 modelMatrix;
    float4x4 viewMatrix;
    float4x4 projectionMatrix;
};

StructuredBuffer<float3> vertexBuffer : register(t0);
StructuredBuffer<float3> normalBuffer : register(t1);

VS_Output vs_main(VS_Input input)
{
    VS_Output output;


    float3 pos = input.position.xzy;

    output.position = mul(float4(pos, 1.f), modelMatrix);
    output.position = mul(output.position, viewMatrix);
    output.position = mul(output.position, projectionMatrix);

    output.worldPosition = mul(float4(pos, 1.f), modelMatrix).xyz;

    output.color = float4(0.5f, 0.5f, 1.f, 1.f);

    output.normal = normalize(mul(input.normal.xzy, (float3x3)modelMatrix));
    
    output.uv = input.uv;

    return output;
}
