cbuffer PerFrame : register(b1)
{
    float3 L;       float pad0;
    float3 Lc;      float pad1;
    float3 Ac;      float pad2;
    float3 viewPos; float pad3;
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