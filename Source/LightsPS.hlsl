#include "Lights.hlsli"

Texture2D diffuseTex : register(t0);
SamplerState diffuseSamp : register(s0);

static const float PI = 3.14159265f;
static const float EPS = 1e-5f;

float3 SchlickFresnel(float3 F0, float cosTheta)
{
    float x = 1.0f - cosTheta;
    float x2 = x * x;
    float x5 = x2 * x2 * x;
    return F0 + (1.0f - F0) * x5;
}

float EpicAttenuation(float distanceValue, float radiusValue)
{
    if (radiusValue <= EPS)
        return 0.0f;

    float normalizedDistance = distanceValue / radiusValue;
    float normalizedDistance2 = normalizedDistance * normalizedDistance;
    float normalizedDistance4 = normalizedDistance2 * normalizedDistance2;

    float numerator = max(1.0f - normalizedDistance4, 0.0f);
    numerator *= numerator;

    float denominator = distanceValue * distanceValue + 1.0f;
    return numerator / denominator;
}

float SpotConeAttenuation(float cosineAngle, float cosineInner, float cosineOuter)
{
    float denominator = max(cosineInner - cosineOuter, EPS);
    return saturate((cosineAngle - cosineOuter) / denominator);
}

float3 PhongSpecularBRDF(float3 F0, float NdotL, float VdotR, float shininess)
{
    float3 fresnel = SchlickFresnel(F0, NdotL);
    float normalization = (shininess + 2.0f) / (2.0f * PI);
    return normalization * fresnel * pow(VdotR, shininess);
}

float3 EvaluateLight(float3 lightDirection, float3 lightColor, float3 normalVector, float3 viewDirection, float3 F0, float3 diffuseBRDF, float shininess)
{
    float normalDotLight = saturate(-dot(lightDirection, normalVector));
    if (normalDotLight <= 0.0f)
        return 0.0f;

    float3 reflectedLight = reflect(lightDirection, normalVector);
    float viewDotReflected = saturate(dot(viewDirection, reflectedLight));

    float3 specularBRDF = PhongSpecularBRDF(F0, normalDotLight, viewDotReflected, shininess);
    return (diffuseBRDF + specularBRDF) * lightColor * normalDotLight;
}

float3 ComputeDirectionalLight(uint lightIndex, float3 normalVector, float3 viewDirection, float3 F0, float3 diffuseBRDF, float shininess)
{
    float3 lightDirection = normalize(directionalLights[lightIndex].direction);
    float3 lightColor = directionalLights[lightIndex].color * directionalLights[lightIndex].intensity;

    return EvaluateLight(lightDirection, lightColor, normalVector, viewDirection, F0, diffuseBRDF, shininess);
}

float3 ComputePointLight(uint lightIndex, float3 worldPos, float3 normalVector, float3 viewDirection, float3 F0, float3 diffuseBRDF, float shininess)
{
    float3 toSurface = worldPos - pointLights[lightIndex].position; // light -> surface
    float distanceToSurface = length(toSurface);
    if (distanceToSurface <= EPS)
        return 0.0f;

    float3 lightDirection = toSurface / distanceToSurface;

    float attenuation = EpicAttenuation(distanceToSurface, pointLights[lightIndex].radius);
    float3 lightColor = pointLights[lightIndex].color * pointLights[lightIndex].intensity * attenuation;

    return EvaluateLight(lightDirection, lightColor, normalVector, viewDirection, F0, diffuseBRDF, shininess);
}

float3 ComputeSpotLight(uint lightIndex, float3 worldPos, float3 normalVector, float3 viewDirection, float3 F0, float3 diffuseBRDF, float shininess)
{
    float3 spotDirection = normalize(spotLights[lightIndex].direction);

    float3 toSurface = worldPos - spotLights[lightIndex].position; // light -> surface
    float distanceProjected = dot(toSurface, spotDirection);
    if (distanceProjected <= 0.0f)
        return 0.0f;

    float3 lightDirection = normalize(toSurface);

    float attenuation = EpicAttenuation(distanceProjected, spotLights[lightIndex].radius);

    float cosineAngle = dot(lightDirection, spotDirection);
    float coneAttenuation = SpotConeAttenuation(
        cosineAngle,
        spotLights[lightIndex].cosineInnerAngle,
        spotLights[lightIndex].cosineOuterAngle
    );

    float3 lightColor =
        spotLights[lightIndex].color * spotLights[lightIndex].intensity * attenuation * coneAttenuation;

    return EvaluateLight(lightDirection, lightColor, normalVector, viewDirection, F0, diffuseBRDF, shininess);
}

float4 main(float3 worldPos : POSITION, float3 normal : NORMAL, float2 coord : TEXCOORD) : SV_TARGET
{
    float3 albedo = (hasDiffuseTex != 0)
        ? diffuseTex.Sample(diffuseSamp, coord).rgb * diffuseColour
        : diffuseColour;

    float3 normalVector = normalize(normal);
    float3 viewDirection = normalize(viewPos - worldPos);

    float3 F0 = specularColour;

    float maxF0 = max(F0.r, max(F0.g, F0.b));
    float3 albedoEnergy = albedo * (1.0f - maxF0);

    float3 diffuseBRDF = albedoEnergy / PI;

    float3 directLighting = 0.0f;

    // Directional lights
    for (uint i = 0; i < directionalCount; ++i)
    {
        directLighting += ComputeDirectionalLight(i, normalVector, viewDirection, F0, diffuseBRDF, shininess);
    }

    // Point lights
    for (uint i = 0; i < pointCount; ++i)
    {
        directLighting += ComputePointLight(i, worldPos, normalVector, viewDirection, F0, diffuseBRDF, shininess);
    }

    // Spot lights
    for (uint i = 0; i < spotCount; ++i)
    {
        directLighting += ComputeSpotLight(i, worldPos, normalVector, viewDirection, F0, diffuseBRDF, shininess);
    }

    // Ambient
    float3 indirectLighting = ambientColor * ambientIntensity * albedoEnergy;

    return float4(directLighting + indirectLighting, 1.0f);
}
