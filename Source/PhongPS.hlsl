#include "Phong.hlsli"

Texture2D diffuseTex : register(t0);
SamplerState diffuseSamp : register(s0);

static const float PI = 3.14159265f;

float3 SchlickFresnel(float3 F0, float cosTheta)
{
    float x = 1.0f - cosTheta;
    float x2 = x * x;
    float x5 = x2 * x2 * x;
    return F0 + (1.0f - F0) * x5;
}

float EpicAttenuation(float distanceValue, float radiusValue)
{
    if (radiusValue <= 1e-5f)
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
    float denominator = max(cosineInner - cosineOuter, 1e-5f);
    return saturate((cosineAngle - cosineOuter) / denominator);
}

float4 main(float3 worldPos : POSITION, float3 normal : NORMAL, float2 coord : TEXCOORD) : SV_TARGET
{
    float3 Cd = (hasDiffuseTex != 0)
        ? diffuseTex.Sample(diffuseSamp, coord).rgb * diffuseColour
        : diffuseColour;

    float3 N = normalize(normal);
    float3 V = normalize(viewPos - worldPos);

    float3 F0 = specularColour;

    float maxF0 = max(F0.r, max(F0.g, F0.b));
    float3 CdEnergy = Cd * (1.0f - maxF0);

    float3 diffuseBRDF = CdEnergy / PI;

    float3 directLighting = 0.0f;

    // ---------------- Directional lights ----------------
    [loop]
    for (uint i = 0; i < directionalCount; ++i)
    {
        float3 lightDirection = normalize(directionalLights[i].direction);

        float dotNormalLight = saturate(-dot(lightDirection, N));

        float3 reflectedLight = reflect(lightDirection, N);
        float dotViewReflected = saturate(dot(V, reflectedLight));

        float3 fresnel = SchlickFresnel(F0, dotNormalLight);
        float3 specularBRDF =
            ((shininess + 2.0f) / (2.0f * PI)) * fresnel * pow(dotViewReflected, shininess);

        float3 lightColor = directionalLights[i].color * directionalLights[i].intensity;

        directLighting += (diffuseBRDF + specularBRDF) * lightColor * dotNormalLight;
    }

    // ---------------- Point lights ----------------
    [loop]
    for (uint i = 0; i < pointCount; ++i)
    {
        float3 toSurface = worldPos - pointLights[i].position;
        float distanceToSurface = length(toSurface);
        if (distanceToSurface <= 1e-5f)
            continue;

        float3 lightDirection = toSurface / distanceToSurface;
        float dotNormalLight = saturate(-dot(lightDirection, N));

        float3 reflectedLight = reflect(lightDirection, N);
        float dotViewReflected = saturate(dot(V, reflectedLight));

        float3 fresnel = SchlickFresnel(F0, dotNormalLight);
        float3 specularBRDF =
            ((shininess + 2.0f) / (2.0f * PI)) * fresnel * pow(dotViewReflected, shininess);

        float attenuation = EpicAttenuation(distanceToSurface, pointLights[i].radius);
        float3 lightColor = pointLights[i].color * pointLights[i].intensity * attenuation;

        directLighting += (diffuseBRDF + specularBRDF) * lightColor * dotNormalLight;
    }

    // ---------------- Spot lights ----------------
    [loop]
    for (uint i = 0; i < spotCount; ++i)
    {
        float3 spotDirection = normalize(spotLights[i].direction);

        float3 toSurface = worldPos - spotLights[i].position;

        float distanceProjected = dot(toSurface, spotDirection);
        if (distanceProjected <= 0.0f)
            continue;

        float3 rayDirection = normalize(toSurface);
        float dotNormalLight = saturate(-dot(rayDirection, N));

        float3 reflectedLight = reflect(rayDirection, N);
        float dotViewReflected = saturate(dot(V, reflectedLight));

        float3 fresnel = SchlickFresnel(F0, dotNormalLight);
        float3 specularBRDF =
            ((shininess + 2.0f) / (2.0f * PI)) * fresnel * pow(dotViewReflected, shininess);

        float attenuation = EpicAttenuation(distanceProjected, spotLights[i].radius);

        float cosineAngle = dot(rayDirection, spotDirection);
        float coneAttenuation = SpotConeAttenuation(
            cosineAngle,
            spotLights[i].cosineInnerAngle,
            spotLights[i].cosineOuterAngle
        );

        float3 lightColor =
            spotLights[i].color * spotLights[i].intensity * attenuation * coneAttenuation;

        directLighting += (diffuseBRDF + specularBRDF) * lightColor * dotNormalLight;
    }

    // ---------------- Ambient ----------------
    float3 indirectLighting = ambientColor * ambientIntensity * CdEnergy;

    return float4(directLighting + indirectLighting, 1.0f);
}
