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

Texture2D SurfaceTexture : register(t0);    // "t" registers for textures
Texture2D OverlayTexture : register(t1);
SamplerState BasicSampler : register(s0);   // "s" registers for samplers

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
	// Scale and offset the uv
    input.uv = input.uv * scale + offset;
	
	// Sampling from the surface texture to get a color
    float4 surfaceColor = SurfaceTexture.Sample(BasicSampler, input.uv);
    
    // Sampling from the overlay texture to get a color
    float4 overlayColor = OverlayTexture.Sample(BasicSampler, input.uv);
    
    // Combine surface and overlay
    float4 combinedColor = surfaceColor * overlayColor;
	
	// Apply color tint
    return combinedColor * colorTint;
}