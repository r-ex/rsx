struct VS_Input
{
    float3 position : POSITION;
    uint normal : NORMAL;
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
    
    output.color = float4((input.color & 0xff) / 255.f, ((input.color << 8) & 0xff) / 255.f, ((input.color << 16) & 0xff) / 255.f, ((input.color << 24) & 0xff) / 255.f);

    // todo: check if normal needs to be modified in any way like the position
    bool sign = (input.normal >> 28) & 1;

    float val1 = sign ? -255.f : 255.f; // normal value 1
    float val2 = ((input.normal >> 19) & 0x1FF) - 256.f;
    float val3 = ((input.normal >> 10) & 0x1FF) - 256.f;

    int idx1 = (input.normal >> 29) & 3;
    int idx2 = (0x124u >> (2 * idx1 + 2)) & 3;
    int idx3 = (0x124u >> (2 * idx1 + 4)) & 3;

	// normalise the normal
    float normalised = rsqrt((255.f * 255.f) + (val2 * val2) + (val3 * val3));

    float3 vals = float3(val1 * normalised, val2 * normalised, val3 * normalised);
    float3 tmp;
    
    // indices are sequential, always:
    // 0 1 2
    // 2 0 1
    // 1 2 0
    
    if (idx1 == 0)
    {
        tmp.x = vals[idx1];
        tmp.y = vals[idx2];
        tmp.z = vals[idx3];
    }
    else if (idx1 == 1)
    {
        tmp.x = vals[idx3];
        tmp.y = vals[idx1];
        tmp.z = vals[idx2];
    }
    else if (idx1 == 2)
    {
        tmp.x = vals[idx2];
        tmp.y = vals[idx3];
        tmp.z = vals[idx1];
    }
    
    output.normal = normalize(mul(tmp, modelMatrix));
    
    output.uv = input.uv;

    return output;
}
