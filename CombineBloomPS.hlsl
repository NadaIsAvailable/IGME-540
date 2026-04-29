struct VertexToPixel
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD0;
};

Texture2D Original : register(t0);
Texture2D Brighten : register(t1);
SamplerState ClampSampler : register(s0);

float4 main(VertexToPixel input) : SV_TARGET
{
    float4 originalTexture = Original.Sample(ClampSampler, input.uv);
    float4 brightenTexture = Brighten.Sample(ClampSampler, input.uv);
    
    float4 finalColor = originalTexture + brightenTexture;
    return finalColor;
}