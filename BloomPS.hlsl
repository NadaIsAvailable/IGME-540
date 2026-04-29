#define BLOOM_TYPE_AVERAGE 0
#define BLOOM_TYPE_LIGHTNESS 1
#define BLOOM_TYPE_LUMINANCE 2

cbuffer externalData : register(b0)
{
    float bloomThreshold;
    int bloomType;
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
    float gray = 0;
    
    switch (bloomType)
    {
        case BLOOM_TYPE_AVERAGE:
            // (R + G + B) / 3
            float average = (color.r + color.g + color.b) / 3;
        
            if (average >= bloomThreshold)
                // soft falloff
                gray = max(average - bloomThreshold, 0.0f);
            break;
        case BLOOM_TYPE_LIGHTNESS:
            // (max(R, G, B) + min(R, G, B)) / 2
            float maxVal = max(color.r, max(color.g, color.b));
            float minVal = min(color.r, min(color.g, color.b));
            float lightness = (maxVal + minVal) / 2;
        
            if (lightness >= bloomThreshold)
                // soft falloff
                gray = max(lightness - bloomThreshold, 0.0f);
            break;
        case BLOOM_TYPE_LUMINANCE:
            // 0.21 * R + 0.72 * G + 0.07 * B
            float luminance = dot(color.rgb, float3(0.21f, 0.72f, 0.07f));
            
            if (luminance >= bloomThreshold)
                // soft falloff
                gray = max(luminance - bloomThreshold, 0.0f);
            break;
        default:
            break;
    }
    
    return float4(gray, gray, gray, 1);
}