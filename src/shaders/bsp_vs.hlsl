struct VS_Input
{
    float3 position : POSITION;
    float3 normal : NORMAL;
    uint color : COLOR;
    float2 uv : TEXCOORD;    
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

VS_Output vs_main(VS_Input input)
{
    VS_Output output;


    // i am really not sure how to make the coordinate system natively behave in the same way as source
    // since source is +z up and basically every other dx or ogl app uses +y up
    // so "for now" (it will end up staying like this most likely) we have to switch the coordinates around here
    // i hate this so much
    // float3 pos = input.position.xzy;
    
    // either have to use xzy or -x y z to get accurate uvs
    float3 pos = float3(-input.position.x, input.position.y, input.position.z);

    output.position = mul(float4(pos, 1.f), modelMatrix);
    output.position = mul(output.position, viewMatrix);
    output.position = mul(output.position, projectionMatrix);

    output.worldPosition = mul(float4(input.position, 1.f), modelMatrix);

    //output.color = input.color;
    
    output.color = float4((input.color & 0xff) / 255.f, ((input.color >> 8) & 0xff) / 255.f, ((input.color >> 16) & 0xff) / 255.f, ((input.color >> 24) & 0xff) / 255.f);

    output.normal = normalize(mul(input.normal, modelMatrix));
    
    output.uv = input.uv;

    return output;
}
