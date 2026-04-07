#include "ShaderStructsIncludes.hlsli"
#include "ShaderLightingIncludes.hlsli"

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
Texture2D NormalMap : register(t1);
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
    // renoramlize the normal after going through the Rasterizer
    input.normal = normalize(input.normal);
    input.tangent = normalize(input.tangent);
    
    // Unpack normal from texture sample – ensure normalization!
    float3 normalFromMap = NormalMap.Sample(BasicSampler, input.uv).rgb;
    float3 unpackedNormal = normalize(normalFromMap * 2.0f - 1.0f);
    
    // Create TBN matrix
    float3 N = normalize(input.normal);
    float3 T = normalize(input.tangent - dot(input.tangent, N) * N); // Orthonormalize
    float3 B = cross(T, N);
    float3x3 TBN = float3x3(T, B, N);
    
    // Transform normal from map using TBN
    float3 finalNormal = mul(unpackedNormal, TBN);
    
	// Scale and offset the uv
    input.uv = input.uv * scale + offset;
	
	// Sampling from the surface texture to get a color and apply color tint
    float3 surfaceColor = SurfaceTexture.Sample(BasicSampler, input.uv).rgb;
    surfaceColor *= colorTint.rgb;
    
    // Add ambient to final light (just once)
    float3 totalLight = ambientColor;
    
    // Loop through each light source
    for (int i = 0; i < lightCount; i++)
    {
        Light light = lights[i];
        
        // Lighting calculation variables
        float3 toCamera = normalize(cameraPos - input.worldPosition);
        
        // Add result from each light to the final lighting result
        switch (light.Type)
        {
            case LIGHT_TYPE_DIRECTIONAL:
                totalLight += DirLight(light, finalNormal, toCamera, surfaceColor);
                break;
            case LIGHT_TYPE_POINT:
                totalLight += PointLight(light, finalNormal, input.worldPosition, toCamera, surfaceColor);
                break;
            case LIGHT_TYPE_SPOT:
                totalLight += SpotLight(light, finalNormal, input.worldPosition, toCamera, surfaceColor);
                break;
        }
    }
    
    return float4(totalLight, 1);
}