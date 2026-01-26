cbuffer PerFrame : register(b1)
{
    float3 viewPos;
    float pad0;
};

cbuffer PerInstance : register(b2)
{
    float4x4 modelMat;
    float4x4 normalMat;
    
    float3 diffuseColour;
    uint hasDiffuseTex;

    float3 specularColour;
    float shininess;
};

#define MAX_DIRECTIONAL_LIGHTS 4
#define MAX_POINT_LIGHTS 32
#define MAX_SPOT_LIGHTS 16

struct DirectionalLight
{
    float3 direction;
    float pad0;
    float3 color;
    float intensity;
};

struct PointLight
{
    float3 position;
    float radius;
    float3 color;
    float intensity;
};

struct SpotLight
{
    float3 position;
    float radius;
    float3 direction;
    float pad0;
    float3 color;
    float intensity;

    float cosineInnerAngle;
    float cosineOuterAngle;
    float2 pad1;
};

cbuffer LightsCB : register(b3)
{
    float3 ambientColor;
    float ambientIntensity;

    uint directionalCount;
    uint pointCount;
    uint spotCount;
    uint paddingCounts;

    DirectionalLight directionalLights[MAX_DIRECTIONAL_LIGHTS];
    PointLight pointLights[MAX_POINT_LIGHTS];
    SpotLight spotLights[MAX_SPOT_LIGHTS];
};