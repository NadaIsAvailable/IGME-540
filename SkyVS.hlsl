#include "StructsIncludes.hlsli"

cbuffer ExternalData : register(b0)
{
    float4x4 view;
    float4x4 projection;
}

VertexToPixel_Skybox main(VertexShaderInput input)
{
	// Set up output struct
    VertexToPixel_Skybox output;
    
    float4x4 viewNoTranslation = view;
    
    // remove translation from view matrix
    viewNoTranslation._14 = 0;
    viewNoTranslation._24 = 0;
    viewNoTranslation._34 = 0;
    
    // apply the view and projection matrices to the position
    output.screenPosition = mul(projection, mul(viewNoTranslation, float4(input.localPosition, 1.0f)));
    // To ensure that the output depth of each vertex will be exactly 1.0 after the shader
    // Set the output position's Z value equal to its W value
    output.screenPosition.z = output.screenPosition.w;
    
    output.sampleDir = input.localPosition;
    
    return output;
}