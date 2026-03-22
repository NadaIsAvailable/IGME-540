cbuffer ExternalData : register(b0)
{
    float4 colorTint;
    float totalTime;
    float3 intensities;
    float2 scale;
    float2 offset;
}

Texture2D SurfaceTexture : register(t0); // "t" registers for textures
Texture2D OverlayTexture : register(t1); // "t" registers for textures
SamplerState BasicSampler : register(s0); // "s" registers for samplers

// Struct representing the data we expect to receive from earlier pipeline stages
// - Should match the output of our corresponding vertex shader
// - The name of the struct itself is unimportant
// - The variable names don't have to match other shaders (just the semantics)
// - Each variable must have a semantic, which defines its usage
struct VertexToPixel
{
	// Data type
	//  |
	//  |   Name          Semantic
	//  |    |                |
	//  v    v                v
    float4 screenPosition : SV_POSITION;
    float2 uv : TEXCOORD;
    float3 normal : NORMAL;
};

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