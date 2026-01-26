#pragma once

#include <variant>
#include <vector>
#include <cstdint>
#include <SimpleMath.h>

using DirectX::SimpleMath::Vector3;


struct LightCommon
{
    bool enabled = true;
    Vector3 color = Vector3::One;
    float intensity = 1.0f;
};

struct DirectionalLightParameters
{

};

struct PointLightParameters
{
    float radius = 10.0f;
};

struct SpotLightParameters
{
    float radius = 10.0f;
    float innerAngleDegrees = 20.0f;
    float outerAngleDegrees = 30.0f;
};

using LightParametersVariant = std::variant<
    DirectionalLightParameters,
    PointLightParameters,
    SpotLightParameters
>;

struct LightComponent
{
    LightCommon common;
    LightParametersVariant parameters;
};

struct ManualTransform
{
    Vector3 position = Vector3::Zero;
    Vector3 forward = Vector3::Forward;
};

using LightId = uint32_t;
using OwnerId = uint32_t;

struct LightInstance
{
    OwnerId ownerId = 0;
    LightComponent lightComponent;
};

struct GPUDirectionalLight
{
    Vector3 direction; float padding0 = 0.0f;
    Vector3 color;     float intensity = 1.0f;
};

struct GPUPointLight
{
    Vector3 position;  float radius = 10.0f;
    Vector3 color;     float intensity = 1.0f;
};

struct GPUSpotLight
{
    Vector3 position;  float radius = 10.0f;
    Vector3 direction; float padding0 = 0.0f;
    Vector3 color;     float intensity = 1.0f;

    float cosineInnerAngle = 0.0f;
    float cosineOuterAngle = 0.0f;
    float padding1[2] = { 0.0f, 0.0f };
};

struct PackedLightsGPU
{
    std::vector<GPUDirectionalLight> directionalLights;
    std::vector<GPUPointLight> pointLights;
    std::vector<GPUSpotLight> spotLights;

    Vector3 ambientColor = Vector3(0.1f, 0.1f, 0.1f);
    float ambientIntensity = 1.0f;
};

static constexpr uint32_t MAX_DIRECTIONAL_LIGHTS = 4;
static constexpr uint32_t MAX_POINT_LIGHTS = 32;
static constexpr uint32_t MAX_SPOT_LIGHTS = 16;

struct GPULightsConstantBuffer
{
    Vector3 ambientColor; float ambientIntensity = 1.0f;

    uint32_t directionalCount = 0;
    uint32_t pointCount = 0;
    uint32_t spotCount = 0;
    uint32_t paddingCounts = 0;

    GPUDirectionalLight directionalLights[MAX_DIRECTIONAL_LIGHTS]{};
    GPUPointLight       pointLights[MAX_POINT_LIGHTS]{};
    GPUSpotLight        spotLights[MAX_SPOT_LIGHTS]{};
};