#pragma once

#include <vector>
#include <cstdint>
#include <SimpleMath.h>

using DirectX::SimpleMath::Vector3;

using LightId = uint32_t;
using OwnerId = uint32_t;

struct LightDefaults
{
    static constexpr float DEFAULT_INTENSITY = 1.0f;

    static constexpr float DEFAULT_POINT_RADIUS = 10.0f;

    static constexpr float DEFAULT_SPOT_RADIUS = 10.0f;
    static constexpr float DEFAULT_SPOT_INNER_ANGLE_DEGREES = 20.0f;
    static constexpr float DEFAULT_SPOT_OUTER_ANGLE_DEGREES = 30.0f;

    static constexpr float DEFAULT_AMBIENT_INTENSITY = 1.0f;
    static constexpr Vector3 DEFAULT_AMBIENT_COLOR = Vector3(0.1f, 0.1f, 0.1f);

    static constexpr uint32_t MAX_DIRECTIONAL_LIGHTS = 4;
    static constexpr uint32_t MAX_POINT_LIGHTS = 32;
    static constexpr uint32_t MAX_SPOT_LIGHTS = 16;
};

enum class LightType : uint8_t
{
    DIRECTIONAL = 0,
    POINT = 1,
    SPOT = 2
};

struct LightCommon
{
    bool enabled = true;
    Vector3 color = Vector3::One;
    float intensity = LightDefaults::DEFAULT_INTENSITY;
};

struct DirectionalLightParameters
{
};

struct PointLightParameters
{
    float radius = LightDefaults::DEFAULT_POINT_RADIUS;
};

struct SpotLightParameters
{
    float radius = LightDefaults::DEFAULT_SPOT_RADIUS;
    float innerAngleDegrees = LightDefaults::DEFAULT_SPOT_INNER_ANGLE_DEGREES;
    float outerAngleDegrees = LightDefaults::DEFAULT_SPOT_OUTER_ANGLE_DEGREES;
};

struct LightParameters
{
    DirectionalLightParameters directional{};
    PointLightParameters       point{};
    SpotLightParameters        spot{};

    static LightParameters makeDirectional()
    {
        LightParameters parameters{};
        parameters.directional = {};
        return parameters;
    }

    static LightParameters makePoint(float radius)
    {
        LightParameters parameters{};
        parameters.point = { radius };
        return parameters;
    }

    static LightParameters makeSpot(float radius, float innerAngleDegrees, float outerAngleDegrees)
    {
        LightParameters parameters{};
        parameters.spot = { radius, innerAngleDegrees, outerAngleDegrees };
        return parameters;
    }
};

struct LightComponent
{
    LightCommon common{};
    LightType type = LightType::DIRECTIONAL;
    LightParameters parameters = LightParameters::makeDirectional();
};

struct ManualTransform
{
    Vector3 position = Vector3::Zero;
    Vector3 forward = Vector3::Forward;
};

struct LightInstance
{
    OwnerId ownerId = 0;
    LightComponent lightComponent;
};

struct GPUDirectionalLight
{
    Vector3 direction{};
    float padding0 = 0.0f;

    Vector3 color{};
    float intensity = LightDefaults::DEFAULT_INTENSITY;
};

struct GPUPointLight
{
    Vector3 position{};
    float radius = LightDefaults::DEFAULT_POINT_RADIUS;

    Vector3 color{};
    float intensity = LightDefaults::DEFAULT_INTENSITY;
};

struct GPUSpotLight
{
    Vector3 position{};
    float radius = LightDefaults::DEFAULT_SPOT_RADIUS;

    Vector3 direction{};
    float padding0 = 0.0f;

    Vector3 color{};
    float intensity = LightDefaults::DEFAULT_INTENSITY;

    float cosineInnerAngle = 0.0f;
    float cosineOuterAngle = 0.0f;
    float padding1[2] = { 0.0f, 0.0f };
};

struct PackedLightsGPU
{
    std::vector<GPUDirectionalLight> directionalLights;
    std::vector<GPUPointLight> pointLights;
    std::vector<GPUSpotLight> spotLights;

    Vector3 ambientColor = LightDefaults::DEFAULT_AMBIENT_COLOR;
    float ambientIntensity = LightDefaults::DEFAULT_AMBIENT_INTENSITY;
};

struct GPULightsConstantBuffer
{
    Vector3 ambientColor = LightDefaults::DEFAULT_AMBIENT_COLOR;
    float ambientIntensity = LightDefaults::DEFAULT_AMBIENT_INTENSITY;

    uint32_t directionalCount = 0;
    uint32_t pointCount = 0;
    uint32_t spotCount = 0;
    uint32_t paddingCounts = 0;

    GPUDirectionalLight directionalLights[LightDefaults::MAX_DIRECTIONAL_LIGHTS];
    GPUPointLight pointLights[LightDefaults::MAX_POINT_LIGHTS];
    GPUSpotLight spotLights[LightDefaults::MAX_SPOT_LIGHTS];
};