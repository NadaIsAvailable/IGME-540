#ifndef __GGP_LIGHTING_INCLUDES__ // Each .hlsli file needs a unique identifier!
#define __GGP_LIGHTING_INCLUDES__

#define LIGHT_TYPE_DIRECTIONAL 0
#define LIGHT_TYPE_POINT 1
#define LIGHT_TYPE_SPOT 2

#define MIN_ROUGHNESS 0.0000001
#define PI 3.1415926538

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

// Diffuse: Lambertian reflectance
float Lambert(float3 normal, float3 toLight)
{
    // Calculate the diffuse intensity
    float lambert = dot(normal, toLight);
    float diffuse = saturate(lambert); // clamp to 0 - 1
    return diffuse;
}

// Specular: Phong reflection
float Phong(float3 toCamera, float3 lightDir, float3 normal, float shininess = 200)
{
    // Reflection of the incoming light
    float3 reflection = reflect(lightDir, normal);
    
    // Cosine of reflection and camera (clamp to 0 - 1)
    float RdotV = saturate(dot(reflection, toCamera));
    
    // Apply shininess
    return pow(RdotV, shininess);
}

// Normal Distribution Function : GGX (Trowbridge-Reitz)
// - n: normal
// - h: half vector
float D_GGX(float3 n, float3 h, float roughness)
{
    // Pre-calculations
    float NdotH = saturate(dot(n, h));
    float NdotH2 = NdotH * NdotH;
    
    // Remap roughness (Unreal & Disney)
    // bc roughness from 0.5 - 1 "seems" to have little visual difference
    float a = roughness * roughness;
    float a2 = max(a * a, MIN_ROUGHNESS); // MIN_ROUGHNESS is 0.0000001
    
    // everything in the denominator beside PI and the outter square
    float denomToSquare = NdotH2 * (a2 - 1) + 1;
    
    // Final value
    return a2 / (PI * denomToSquare * denomToSquare);
}

// Geometry Shadowing: Schlick-GGX
// - n: normal
// - v: view direction (view vector/light direction)
// This is called twice and combined: G(n,v,r) * G(n,l,r)
// Note: Roughness is remapped to match artist expectations & other math
float G_SchlickGGX(float3 n, float3 v, float roughness)
{
    // End result of all roughness remapping
    float k = pow(roughness + 1, 2) / 8.0f;
    float NdotV = saturate(dot(n, v));
   
    // Note: Numerator (NdotV) is now 1 to cancel Cook-Torrance denominator
    return 1 / (NdotV * (1 - k) + k);
}

// Fresnel: Schlick's approximation
// - v: view vector (to camera)
// - h: half vector
// - f0: reflectance at normal incidence (0 degrees)
// f0 ranges from 0.04 for non-metals to
// a specific specular color for metals
float3 F_Schlick(float3 v, float3 h, float3 f0)
{
    // Pre-calculations
    float VdotH = saturate(dot(v, h));
    
    // Final value – Schlick’s approximation
    return f0 + (1 - f0) * pow(1 - VdotH, 5);
}

// Specular: Cook-Torrance Microfacet BRDF
float3 MicrofacetBRDF(float3 n, float3 l, float3 v, float roughness, float3 f0)
{
    // Calculate the half vector
    float3 h = normalize((l + v) / 2);
    
    // Calculate each component of the microfacet BRDF
    float D = D_GGX(n, h, roughness);
    float3 F = F_Schlick(v, h, f0);
    float G = G_SchlickGGX(n, v, roughness) * G_SchlickGGX(n, l, roughness);
    
    // Final specular formula
    // Note: (NdotV)(NdotL) in the deminator has been canceled out by optimized G()
    float3 specularResult = (D * F * G) / 4;
    
    // N dot L term for BRDF
    return specularResult * saturate(dot(n, l));
}

// Calculates attentuation of lights
float Attenuate(Light light, float3 worldPos)
{
    float dist = distance(light.Position, worldPos);
    float att = saturate(1.0f - (dist * dist / (light.Range * light.Range)));
    return att * att;
}

// Calculates diffuse color based on energy conservation
// Note: Metal albedo is approximately (0,0,0), so cut diffuse for metals
// Note: This is incredibly basic energy conservation
float3 DiffuseEnergyConserve(
    float3 diffuse,
    float3 F, // Should be result of just F_Schlick()
    float metalness)
{
    return diffuse * (1 - F) * (1 - metalness);
}


// ============================================================================
// Types of Lightings
// ============================================================================

// Calculates total light for a directional light
float3 DirLight(
    Light light, 
    float3 normal, 
    float3 toCamera, 
    float roughness, 
    float metalness, 
    float3 f0, 
    float3 surfaceColor)
{
    // Calculate vector towards light source
    float3 toLight = normalize(-light.Direction);
    
    // Lighting calculation
    float diffuse = Lambert(normal, toLight);
    // float specular = Phong(toCamera, -toLight, normal, 24);
    float3 specular = MicrofacetBRDF(normal, toLight, toCamera, roughness, f0);
    
    // Cut the specular if the diffuse contribution is zero
    // - any() returns 1 if any component of the param is non-zero
    specular *= any(diffuse);
    
    // Calculate diffuse with energy conservation, including cutting diffuse for metals
    float3 h = normalize((toLight + toCamera) / 2);
    float3 F = F_Schlick(toCamera, h, f0);
    float3 balancedDiff = DiffuseEnergyConserve(diffuse, F, metalness);
    
    // Combine each light calculation for the final lighting
    // (surfaceColor * diffuse + specular) * ... -> not applying surfaceColor to reflection
    // (diffuse + specular) * ... * surfaceColor -> applying surfaceColor to reflection
    return (surfaceColor * balancedDiff + specular) * light.Color * light.Intensity;
}

// Calculates total light for a point light
float3 PointLight(
    Light light, 
    float3 normal, 
    float3 worldPosition, 
    float3 toCamera, 
    float roughness, 
    float metalness, 
    float3 f0, 
    float3 surfaceColor)
{
    // Calculate to light vector from each point on the surface
    float3 toLight = normalize(light.Position - worldPosition);
    
    // Attenutation calculation
    float attenuate = Attenuate(light, worldPosition);
    
    // Lighting calculation
    float diffuse = Lambert(normal, toLight);
    // float specular = Phong(toCamera, -toLight, normal, 24);
    float3 specular = MicrofacetBRDF(normal, toLight, toCamera, roughness, f0);
    
    // Cut the specular if the diffuse contribution is zero
    // - any() returns 1 if any component of the param is non-zero
    specular *= any(diffuse);

    // Calculate diffuse with energy conservation, including cutting diffuse for metals
    float3 h = normalize((toLight + toCamera) / 2);
    float3 F = F_Schlick(toCamera, h, f0);
    float3 balancedDiff = DiffuseEnergyConserve(diffuse, F, metalness);
    
    // Combine each light calculation for the final lighting
    // (surfaceColor * diffuse + specular) * ... -> not applying surfaceColor to reflection
    // (diffuse + specular) * ... * surfaceColor -> applying surfaceColor to reflection
    return (surfaceColor * balancedDiff + specular) * light.Color * light.Intensity;
}

// Calculates total light for a spot light
float3 SpotLight(
    Light light,
    float3 normal,
    float3 worldPosition,
    float3 toCamera,
    float roughness,
    float metalness,
    float3 f0,
    float3 surfaceColor)
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
    return PointLight(light, normal, worldPosition, toCamera, roughness, metalness, f0, surfaceColor) * spotTerm;
}

#endif