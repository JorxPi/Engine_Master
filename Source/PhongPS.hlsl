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

float4 main(float3 worldPos : POSITION, float3 normal : NORMAL, float2 coord : TEXCOORD) : SV_TARGET
{
    float3 Cd = (hasDiffuseTex != 0) ? diffuseTex.Sample(diffuseSamp, coord).rgb * diffuseColour : diffuseColour;

    float3 N = normalize(normal);
    float3 V = normalize(viewPos - worldPos);

    float3 Ldir = normalize(L);
    float dotNL = saturate(-dot(Ldir, N));
    float3 R = reflect(Ldir, N);
    float dotVR = saturate(dot(V, R));

    float3 F0 = specularColour;
    float3 F = SchlickFresnel(F0, dotNL);

    float maxF0 = max(F0.r, max(F0.g, F0.b));
    float3 CdEnergy = Cd * (1.0f - maxF0);

    float3 diffuseBRDF = CdEnergy / PI;
    float3 specularBRDF = ((shininess + 2.0f) / (2.0f * PI)) * F * pow(dotVR, shininess);

    float3 direct = (diffuseBRDF + specularBRDF) * Lc * dotNL;
    float3 indirect = Ac * CdEnergy;

    return float4(direct + indirect, 1.0f);
}