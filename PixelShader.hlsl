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
    //float3 ambientColor;
    //float padding2;
    Light lights[MAX_LIGHTS];
    int lightCount;
}

Texture2D Albedo : register(t0);
Texture2D NormalMap : register(t1);
Texture2D RoughnessMap : register(t2);
Texture2D MetalnessMap : register(t3);
SamplerState BasicSampler : register(s0);

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
    // renoramlize the normal and the tangent after going through the RS
    input.normal = normalize(input.normal);
    input.tangent = normalize(input.tangent);
    
    // Unpack normal from texture sample – ensure normalization
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
	
	// Sampling from the albedo texture
    float3 albedoColor = Albedo.Sample(BasicSampler, input.uv).rgb;
    // Ungamma corrected texture
    albedoColor = pow(albedoColor, 2.2f);
    // Apply color tint to the albedo color
    albedoColor *= colorTint.rgb;
    
    // Sample roughness and metalness
    float roughness = RoughnessMap.Sample(BasicSampler, input.uv).r;
    float metalness = MetalnessMap.Sample(BasicSampler, input.uv).r;
    
    // Specular color (f0) determination -----------------
    // Assume albedo texture is actually holding specular color where metalness == 1
    // Note the use of lerp here - metal is generally 0 or 1, but might be in between
    // because of linear texture sampling, so we lerp the specular color to match
    float3 f0 = lerp(0.04f, albedoColor.rgb, metalness);
    
    // Add ambient to final light (just once)
    // float3 totalLight = albedoColor;
    float3 totalLight = float3(0, 0, 0);
    
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
                totalLight += DirLight(light, finalNormal, toCamera, roughness, metalness, f0, albedoColor);
                break;
            case LIGHT_TYPE_POINT:
                totalLight += PointLight(light, finalNormal, input.worldPosition, toCamera, roughness, metalness, f0, albedoColor);
                break;
            case LIGHT_TYPE_SPOT:
                totalLight += SpotLight(light, finalNormal, input.worldPosition, toCamera, roughness, metalness, f0, albedoColor);
                break;
        }
    }
    
    // Gamma correction
    float4 gammaCorrected = float4(pow(totalLight, 1.0f / 2.2f), 1);
    
    return gammaCorrected;
}