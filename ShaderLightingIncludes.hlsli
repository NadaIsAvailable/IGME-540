#ifndef __GGP_SHADER_LIGHTING_INCLUDES__ // Each .hlsli file needs a unique identifier!
#define __GGP_SHADER_LIGHTING_INCLUDES__

#define LIGHT_TYPE_DIRECTIONAL 0
#define LIGHT_TYPE_POINT 1
#define LIGHT_TYPE_SPOT 2

struct Light
{
    int Type;               // Which kind of light? 0, 1 or 2 (see above)
    float3 Direction;       // Directional and Spot lights need a direction
    float Range;            // Point and Spot lights have a max range for attenuation
    float3 Position;        // Point and Spot lights have a position in space
    float Intensity;        // All lights need an intensity
    float3 Color;           // All lights need a color
    float SpotInnerAngle;   // Inner cone angle (in radians) – Inside this, full light!
    float SpotOuterAngle;   // Outer cone angle (radians) – Outside this, no light!
    float2 Padding;         // Purposefully padding to hit the 16-byte boundary
};

// ============================================================================
// Light Calculation Helper Methods
// ============================================================================

// Calculates the diffuse intensity
float Diffuse(float3 normal, float3 toLight)
{
    // Calculate the diffuse intensity then clamp to 0 - 1
    float lambert = dot(normal, toLight);
    float diffuseIntensity = saturate(lambert);
    return diffuseIntensity;
}

// Calculates the Phong specular
float Specular(float3 toCamera, float3 lightDir, float3 normal, float shininess = 200)
{
    // Reflection of the incoming light
    float3 reflection = reflect(lightDir, normal);
    
    // Cosine of reflection and camera (clamp to 0 - 1)
    float RdotV = saturate(dot(reflection, toCamera));
    
    // Apply shininess
    return pow(RdotV, shininess);
}

// Calculate attentuation of lights
float Attenuate(Light light, float3 worldPos)
{
    float dist = distance(light.Position, worldPos);
    float att = saturate(1.0f - (dist * dist / (light.Range * light.Range)));
    return att * att;
}

// Calculates total light for a directional light
float3 DirLight(Light light, float3 normal, float3 toCamera, float3 surfaceColor)
{
    // Calculate vector towards light source
    float3 toLight = normalize(-light.Direction);
    
    // Lighting calculation
    float diffuse = Diffuse(normal, toLight);
    float specular = Specular(toCamera, -toLight, normal, 24);
    
    // Combine each light calculation for the final lighting
    return (surfaceColor * (diffuse + specular)) * light.Color * light.Intensity;

}

// Calculates total light for a point light
float3 PointLight(Light light, float3 normal, float3 worldPosition, float3 toCamera, float3 surfaceColor)
{
    // Calculate to light vector from each point on the surface
    float3 toLight = normalize(light.Position - worldPosition);
    
    // Attenutation calculation
    float attenuate = Attenuate(light, worldPosition);
    
    // Lighting calculation
    float diffuse = Diffuse(normal, toLight);
    float specular = Specular(toCamera, -toLight, normal, 24);

    // Combine each light calculation for the final lighting
    return (surfaceColor * (diffuse + specular)) * attenuate * light.Color * light.Intensity;
}

// Calculates total light for a spot light
float3 SpotLight(Light light, float3 normal, float3 worldPosition, float3 toCamera, float3 surfaceColor)
{
    float3 lightToPixel = normalize(worldPosition - light.Position);
    
    // Get cos(angle) between pixel and light direction
    float pixelAngle = saturate(dot(lightToPixel, light.Direction));
    
    // Get cosines of angles and calculate range
    float cosOuter = cos(light.SpotOuterAngle);
    float cosInner = cos(light.SpotInnerAngle);
    float falloffRange = cosOuter - cosInner;
    
    // Linear falloff over the range, clamp 0-1, apply to light calc
    float spotTerm = saturate((cosOuter - pixelAngle) / falloffRange);
    return PointLight(light, normal, worldPosition, toCamera, surfaceColor) * spotTerm;
}

#endif