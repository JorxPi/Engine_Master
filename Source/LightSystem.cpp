#include "Globals.h"
#include "LightSystem.h"
#include <cmath>
#include <algorithm>
#include <type_traits>

static constexpr float kPi = 3.14159265358979323846f;
static constexpr float kNormalizeEpsilon = 1e-8f;

static float degreesToRadians(float degrees)
{
    return degrees * (kPi / 180.0f);
}

Vector3 LightSystem::safeNormalize(const Vector3& vectorToNormalize, const Vector3& fallbackValue)
{
    const float lengthSquared = vectorToNormalize.LengthSquared();
    if (lengthSquared < kNormalizeEpsilon)
        return fallbackValue;

    Vector3 normalizedVector = vectorToNormalize / std::sqrt(lengthSquared);
    return normalizedVector;
}

void LightSystem::sanitizeSpotAngles(float& innerAngleDegrees, float& outerAngleDegrees)
{
    innerAngleDegrees = std::clamp(innerAngleDegrees, 0.0f, 179.0f);
    outerAngleDegrees = std::clamp(outerAngleDegrees, 0.0f, 179.0f);

    if (innerAngleDegrees > outerAngleDegrees)
        std::swap(innerAngleDegrees, outerAngleDegrees);

    if (std::abs(innerAngleDegrees - outerAngleDegrees) < 0.001f)
        outerAngleDegrees = std::min(179.0f, innerAngleDegrees + 0.001f);
}

OwnerId LightSystem::createOwner(const ManualTransform& initialTransform)
{
    const OwnerId newOwnerId = nextOwnerId++;

    ManualTransform ownerTransform = initialTransform;
    ownerTransform.forward = safeNormalize(ownerTransform.forward, Vector3::Forward);

    owners.emplace(newOwnerId, ownerTransform);
    return newOwnerId;
}

bool LightSystem::destroyOwner(OwnerId ownerId)
{
    const auto ownerIterator = owners.find(ownerId);
    if (ownerIterator == owners.end())
        return false;

    owners.erase(ownerIterator);

    for (auto lightIterator = lights.begin(); lightIterator != lights.end();)
    {
        if (lightIterator->second.ownerId == ownerId)
            lightIterator = lights.erase(lightIterator);
        else
            ++lightIterator;
    }

    return true;
}

bool LightSystem::setOwnerTransform(OwnerId ownerId, const ManualTransform& newTransform)
{
    auto ownerIterator = owners.find(ownerId);
    if (ownerIterator == owners.end())
        return false;

    ManualTransform sanitizedTransform = newTransform;
    sanitizedTransform.forward = safeNormalize(sanitizedTransform.forward, Vector3::Forward);

    ownerIterator->second = sanitizedTransform;
    return true;
}

bool LightSystem::getOwnerTransform(OwnerId ownerId, ManualTransform& outputTransform) const
{
    auto ownerIterator = owners.find(ownerId);
    if (ownerIterator == owners.end())
        return false;

    outputTransform = ownerIterator->second;
    return true;
}

bool LightSystem::setOwnerPosition(OwnerId ownerId, const Vector3& newPosition)
{
    auto ownerIterator = owners.find(ownerId);
    if (ownerIterator == owners.end())
        return false;

    ownerIterator->second.position = newPosition;
    return true;
}

bool LightSystem::setOwnerForward(OwnerId ownerId, const Vector3& newForward)
{
    auto ownerIterator = owners.find(ownerId);
    if (ownerIterator == owners.end())
        return false;

    ownerIterator->second.forward = safeNormalize(newForward, Vector3::Forward);
    return true;
}

LightId LightSystem::createLight(OwnerId ownerId, const LightComponent& lightComponent)
{
    if (owners.find(ownerId) == owners.end())
        return 0;

    LightComponent sanitizedLightComponent = lightComponent;

    if (auto* spotLightParameters = std::get_if<SpotLightParameters>(&sanitizedLightComponent.parameters))
        sanitizeSpotAngles(spotLightParameters->innerAngleDegrees, spotLightParameters->outerAngleDegrees);

    const LightId newLightId = nextLightId++;
    lights.emplace(newLightId, LightInstance{ ownerId, sanitizedLightComponent });
    return newLightId;
}

bool LightSystem::destroyLight(LightId lightId)
{
    return lights.erase(lightId) > 0;
}

LightInstance* LightSystem::getLight(LightId lightId)
{
    auto lightIterator = lights.find(lightId);
    return (lightIterator == lights.end()) ? nullptr : &lightIterator->second;
}

const LightInstance* LightSystem::getLight(LightId lightId) const
{
    auto lightIterator = lights.find(lightId);
    return (lightIterator == lights.end()) ? nullptr : &lightIterator->second;
}

LightId LightSystem::createDirectionalLight(OwnerId ownerId, const LightCommon& common, const DirectionalLightParameters& parameters)
{
    LightComponent lightComponent{};
    lightComponent.common = common;
    lightComponent.parameters = parameters;
    return createLight(ownerId, lightComponent);
}

LightId LightSystem::createPointLight(OwnerId ownerId, const LightCommon& common, const PointLightParameters& parameters)
{
    LightComponent lightComponent{};
    lightComponent.common = common;

    PointLightParameters sanitizedParameters = parameters;
    sanitizedParameters.radius = std::max(0.0f, sanitizedParameters.radius);

    lightComponent.parameters = sanitizedParameters;
    return createLight(ownerId, lightComponent);
}

LightId LightSystem::createSpotLight(OwnerId ownerId, const LightCommon& common, const SpotLightParameters& parameters)
{
    LightComponent lightComponent{};
    lightComponent.common = common;

    SpotLightParameters sanitizedParameters = parameters;
    sanitizeSpotAngles(sanitizedParameters.innerAngleDegrees, sanitizedParameters.outerAngleDegrees);
    sanitizedParameters.radius = std::max(0.0f, sanitizedParameters.radius);

    lightComponent.parameters = sanitizedParameters;
    return createLight(ownerId, lightComponent);
}

void LightSystem::setAmbient(const Vector3& ambientColorValue, float ambientIntensityValue)
{
    ambientColor = ambientColorValue;
    ambientIntensity = std::max(0.0f, ambientIntensityValue);
}

PackedLightsGPU LightSystem::packForGPU() const
{
    PackedLightsGPU packedLights;
    packedLights.ambientColor = ambientColor;
    packedLights.ambientIntensity = ambientIntensity;

    for (const auto& lightPair : lights)
    {
        const LightInstance& lightInstance = lightPair.second;
        const LightCommon& common = lightInstance.lightComponent.common;

        if (!common.enabled)
            continue;

        const auto ownerIterator = owners.find(lightInstance.ownerId);
        if (ownerIterator == owners.end())
            continue;

        const ManualTransform& ownerTransform = ownerIterator->second;

        const Vector3 lightForwardDirection = safeNormalize(ownerTransform.forward, Vector3::Forward);

        std::visit(
            [&](auto&& lightParameters)
            {
                using LightParameterType = std::decay_t<decltype(lightParameters)>;

                if constexpr (std::is_same_v<LightParameterType, DirectionalLightParameters>)
                {
                    GPUDirectionalLight gpuDirectionalLight{};
                    gpuDirectionalLight.direction = lightForwardDirection;
                    gpuDirectionalLight.color = common.color;
                    gpuDirectionalLight.intensity = common.intensity;

                    packedLights.directionalLights.push_back(gpuDirectionalLight);
                }
                else if constexpr (std::is_same_v<LightParameterType, PointLightParameters>)
                {
                    GPUPointLight gpuPointLight{};
                    gpuPointLight.position = ownerTransform.position;
                    gpuPointLight.radius = std::max(0.0f, lightParameters.radius);
                    gpuPointLight.color = common.color;
                    gpuPointLight.intensity = common.intensity;

                    packedLights.pointLights.push_back(gpuPointLight);
                }
                else if constexpr (std::is_same_v<LightParameterType, SpotLightParameters>)
                {
                    SpotLightParameters sanitizedSpotLightParameters = lightParameters;
                    sanitizeSpotAngles(sanitizedSpotLightParameters.innerAngleDegrees,
                        sanitizedSpotLightParameters.outerAngleDegrees);

                    GPUSpotLight gpuSpotLight{};
                    gpuSpotLight.position = ownerTransform.position;
                    gpuSpotLight.direction = lightForwardDirection;
                    gpuSpotLight.radius = std::max(0.0f, sanitizedSpotLightParameters.radius);
                    gpuSpotLight.color = common.color;
                    gpuSpotLight.intensity = common.intensity;

                    const float innerAngleRadians = degreesToRadians(sanitizedSpotLightParameters.innerAngleDegrees);
                    const float outerAngleRadians = degreesToRadians(sanitizedSpotLightParameters.outerAngleDegrees);

                    gpuSpotLight.cosineInnerAngle = std::cos(innerAngleRadians);
                    gpuSpotLight.cosineOuterAngle = std::cos(outerAngleRadians);

                    if (gpuSpotLight.cosineInnerAngle < gpuSpotLight.cosineOuterAngle)
                        std::swap(gpuSpotLight.cosineInnerAngle, gpuSpotLight.cosineOuterAngle);

                    packedLights.spotLights.push_back(gpuSpotLight);
                }
            },
            lightInstance.lightComponent.parameters
        );
    }

    return packedLights;
}

GPULightsConstantBuffer LightSystem::packForGPUConstantBuffer() const
{
    GPULightsConstantBuffer constantBuffer{};
    constantBuffer.ambientColor = ambientColor;
    constantBuffer.ambientIntensity = ambientIntensity;

    PackedLightsGPU packedLights = packForGPU();

    constantBuffer.directionalCount = std::min<uint32_t>(
        (uint32_t)packedLights.directionalLights.size(), MAX_DIRECTIONAL_LIGHTS);

    constantBuffer.pointCount = std::min<uint32_t>(
        (uint32_t)packedLights.pointLights.size(), MAX_POINT_LIGHTS);

    constantBuffer.spotCount = std::min<uint32_t>(
        (uint32_t)packedLights.spotLights.size(), MAX_SPOT_LIGHTS);

    for (uint32_t i = 0; i < constantBuffer.directionalCount; ++i)
        constantBuffer.directionalLights[i] = packedLights.directionalLights[i];

    for (uint32_t i = 0; i < constantBuffer.pointCount; ++i)
        constantBuffer.pointLights[i] = packedLights.pointLights[i];

    for (uint32_t i = 0; i < constantBuffer.spotCount; ++i)
        constantBuffer.spotLights[i] = packedLights.spotLights[i];

    return constantBuffer;
}

