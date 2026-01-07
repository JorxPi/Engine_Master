#pragma once
#include <SimpleMath.h>
using namespace DirectX::SimpleMath;

struct PhongMaterialData
{
    XMFLOAT4 diffuseColour;
    float Kd;
    float Ks;
    float shininess;
    BOOL  hasDiffuseTex;
};

struct PBRPhongMaterialData
{
    XMFLOAT3 diffuseColour;
    BOOL     hasDiffuseTex;

    XMFLOAT3 specularColour;
    float    shininess;
};

struct PerInstance
{
    Matrix modelMat;
    Matrix normalMat;
    PBRPhongMaterialData material;
};

struct PerFrame
{
    Vector3 L = Vector3::UnitX; float pad0 = 0.0f;
    Vector3 Lc = Vector3::One;  float pad1 = 0.0f;
    Vector3 Ac = Vector3::Zero; float pad2 = 0.0f;
    Vector3 viewPos = Vector3::Zero; float pad3 = 0.0f;
};

struct PhongSettings
{
    Vector3 lightDir = Vector3(-0.5f, -0.5f, -0.5f);
    Vector3 lightColor = Vector3(1, 1, 1);
    Vector3 ambient = Vector3(0.1f, 0.1f, 0.1f);

    float lightIntensity = 3.0f;
    float ambientIntensity = 1.0f;

    int samplerIndex = 0;

    bool useOverride = false;
    PBRPhongMaterialData overrideMat{};
};
