cbuffer externalData : register(b0)
{
    float bloomThreshold;
}

struct VertexToPixel
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD0;
};

Texture2D Pixels : register(t0);
SamplerState ClampSampler : register(s0);

float4 main(VertexToPixel input) : SV_TARGET
{
    float4 color = Pixels.Sample(ClampSampler, input.uv);
    float luminance = dot(color.rgb, float3(0.21f, 0.72f, 0.07f));
    if (luminance > bloomThreshold)
        return color;
    else
        return float4(0, 0, 0, 1);
}