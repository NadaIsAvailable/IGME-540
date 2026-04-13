#include "StructsIncludes.hlsli"
#include "LightingIncludes.hlsli"

#define MAX_LIGHTS 128

cbuffer ExternalData : register(b0)
{
    float4 colorTint;
    float totalTime;
    float3 intensities;
    float2 scale;
    float2 offset;
    float3 cameraPos;
    float padding1;
    float3 ambientColor;
    float padding2;
    Light lights[MAX_LIGHTS];
    int lightCount;
}

float random(float2 s)
{
    return frac(sin(dot(s, float2(12.9898, 78.233))) * 43758.5453123);
}

// --------------------------------------------------------
// The entry point (main method) for our pixel shader
// 
// - Input is the data coming down the pipeline (defined by the struct)
// - Output is a single color (float4)
// - Has a special semantic (SV_TARGET), which means 
//    "put the output of this into the current render target"
// - Named "main" because that's the default the shader compiler looks for
// --------------------------------------------------------
float4 main(VertexToPixel input) : SV_TARGET
{
    float4 color = float4(0.0f, 0.0f, 0.0f, 1.0f);
    
    float x = sin(input.uv.x * totalTime);
    float y = sin(input.uv.y * totalTime);
    
    color.r = x * intensities.r;
    color.g = y * intensities.g;
    color.b = (x + y) * intensities.b;
    
    return color; 
}