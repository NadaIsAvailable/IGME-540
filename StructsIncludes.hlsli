#ifndef __GGP_STRUCTS_INCLUDES__ // Each .hlsli file needs a unique identifier!
#define __GGP_STRUCTS_INCLUDES__

// Struct representing a single vertex worth of data
struct VertexShaderInput
{
	// Data type
	//  |
	//  |   Name          Semantic
	//  |    |                |
	//  v    v                v
    float3 localPosition	: POSITION; // XYZ position
    float2 uv				: TEXCOORD; // UV coordinates
    float3 normal			: NORMAL;	// Normal vector
    float3 tangent			: TANGENT;	// Tangent vector
};

// Struct representing a single pixel worth of data
struct VertexToPixel
{
	// Data type
	//  |
	//  |   Name          Semantic
	//  |    |                |
	//  v    v                v
    float4 screenPosition	: SV_POSITION;	// XYZW position (System Value Position)
    float2 uv				: TEXCOORD;		// UV coordinates
    float3 normal			: NORMAL;		// Normal vector
    float3 worldPosition	: POSITION;		// World position
    float3 tangent			: TANGENT;		// Tangent vector
};

// Struct representing a single pixel worth of data for sky box
struct VertexToPixel_Skybox
{
	// Data type
	//  |
	//  |   Name          Semantic
	//  |    |                |
	//  v    v                v
    float4 screenPosition	: SV_POSITION;	// XYZW position (System Value Position)
    float3 sampleDir		: DIRECTION;	// Sample direction
};

#endif