#include "Globals.h"
#include "LightSystem.h"

#include <algorithm>
#include <cmath>

namespace
{
    static constexpr float PI = 3.14159265358979323846f;
    static constexpr float NORMALIZE_EPSILON = 1e-8f;

    static constexpr float SPOT_MAX_ANGLE_DEGREES = 179.0f;
    static constexpr float SPOT_MIN_ANGLE_DELTA_DEGREES = 0.001f;
}

Vector3 LightSystem::safeNormalize(const Vector3& vectorToNormalize, const Vector3& fallbackValue)
{
    const float lengthSquared = vectorToNormalize.LengthSquared();
    if (lengthSquared < NORMALIZE_EPSILON)
    {
        return fallbackValue;
    }

    const float inverseLength = 1.0f / std::sqrt(lengthSquared);
    return vectorToNormalize * inverseLength;
}

void LightSystem::sanitizeSpotAngles(float& innerAngleDegrees, float& outerAngleDegrees)
{
    innerAngleDegrees = std::clamp(innerAngleDegrees, 0.0f, SPOT_MAX_ANGLE_DEGREES);
    outerAngleDegrees = std::clamp(outerAngleDegrees, 0.0f, SPOT_MAX_ANGLE_DEGREES);

    if (innerAngleDegrees > outerAngleDegrees)
    {
        std::swap(innerAngleDegrees, outerAngleDegrees);
    }

    if (std::abs(innerAngleDegrees - outerAngleDegrees) < SPOT_MIN_ANGLE_DELTA_DEGREES)
    {
        outerAngleDegrees = std::min(SPOT_MAX_ANGLE_DEGREES, innerAngleDegrees + SPOT_MIN_ANGLE_DELTA_DEGREES);
    }
}

void LightSystem::sanitizeManualTransform(ManualTransform& transform)
{
    transform.forward = safeNormalize(transform.forward, Vector3::Forward);
}

void LightSystem::sanitizeLightComponent(LightComponent& lightComponent)
{
    lightComponent.common.intensity = std::max(0.0f, lightComponent.common.intensity);

    switch (lightComponent.type)
    {
    case LightType::POINT:
    {
        lightComponent.parameters.point.radius = std::max(0.0f, lightComponent.parameters.point.radius);
        break;
    }

    case LightType::SPOT:
    {
        lightComponent.parameters.spot.radius = std::max(0.0f, lightComponent.parameters.spot.radius);

        sanitizeSpotAngles(
            lightComponent.parameters.spot.innerAngleDegrees,
            lightComponent.parameters.spot.outerAngleDegrees
        );
        break;
    }

    case LightType::DIRECTIONAL:
    default:
    {
        break;
    }
    }
}

OwnerId LightSystem::createOwner(const ManualTransform& initialTransform)
{
    const OwnerId newOwnerId = m_nextOwnerId++;

    ManualTransform ownerTransform = initialTransform;
    sanitizeManualTransform(ownerTransform);

    m_owners.emplace(newOwnerId, ownerTransform);
    return newOwnerId;
}

bool LightSystem::destroyOwner(OwnerId ownerId)
{
    const auto ownerIterator = m_owners.find(ownerId);
    if (ownerIterator == m_owners.end())
    {
        return false;
    }

    m_owners.erase(ownerIterator);

    for (auto lightIterator = m_lights.begin(); lightIterator != m_lights.end();)
    {
        if (lightIterator->second.ownerId == ownerId)
        {
            lightIterator = m_lights.erase(lightIterator);
        }
        else
        {
            ++lightIterator;
        }
    }

    return true;
}

bool LightSystem::setOwnerTransform(OwnerId ownerId, const ManualTransform& newTransform)
{
    auto ownerIterator = m_owners.find(ownerId);
    if (ownerIterator == m_owners.end())
    {
        return false;
    }

    ManualTransform sanitizedTransform = newTransform;
    sanitizeManualTransform(sanitizedTransform);

    ownerIterator->second = sanitizedTransform;
    return true;
}

bool LightSystem::getOwnerTransform(OwnerId ownerId, ManualTransform& outputTransform) const
{
    const auto ownerIterator = m_owners.find(ownerId);
    if (ownerIterator == m_owners.end())
    {
        return false;
    }

    outputTransform = ownerIterator->second;
    return true;
}

bool LightSystem::getOwnerTransformFromLight(LightId lightId, ManualTransform& outputTransform) const
{
    const LightInstance* lightInstance = getLight(lightId);
    if (lightInstance == nullptr)
    {
        return false;
    }

    return getOwnerTransform(lightInstance->ownerId, outputTransform);
}

OwnerId LightSystem::getOwnerIdFromLight(LightId lightId) const
{
    const LightInstance* lightInstance = getLight(lightId);
    if (lightInstance == nullptr)
    {
        return 0;
    }

    return lightInstance->ownerId;
}

bool LightSystem::setOwnerTransformFromLight(LightId lightId, const ManualTransform& newTransform)
{
    const LightInstance* lightInstance = getLight(lightId);
    if (lightInstance == nullptr)
    {
        return false;
    }

    return setOwnerTransform(lightInstance->ownerId, newTransform);
}

bool LightSystem::setOwnerPositionFromLight(LightId lightId, const Vector3& newPosition)
{
    const LightInstance* lightInstance = getLight(lightId);
    if (lightInstance == nullptr)
    {
        return false;
    }

    return setOwnerPosition(lightInstance->ownerId, newPosition);
}

bool LightSystem::setOwnerForwardFromLight(LightId lightId, const Vector3& newForward)
{
    const LightInstance* lightInstance = getLight(lightId);
    if (lightInstance == nullptr)
    {
        return false;
    }

    return setOwnerForward(lightInstance->ownerId, newForward);
}

bool LightSystem::setOwnerPosition(OwnerId ownerId, const Vector3& newPosition)
{
    auto ownerIterator = m_owners.find(ownerId);
    if (ownerIterator == m_owners.end())
    {
        return false;
    }

    ownerIterator->second.position = newPosition;
    return true;
}

bool LightSystem::setOwnerForward(OwnerId ownerId, const Vector3& newForward)
{
    auto ownerIterator = m_owners.find(ownerId);
    if (ownerIterator == m_owners.end())
    {
        return false;
    }

    ownerIterator->second.forward = safeNormalize(newForward, Vector3::Forward);
    return true;
}

bool LightSystem::getLightComponent(LightId lightId, LightComponent& outputComponent) const
{
    const LightInstance* lightInstance = getLight(lightId);
    if (lightInstance == nullptr)
    {
        return false;
    }

    outputComponent = lightInstance->lightComponent;
    return true;
}

bool LightSystem::setLightComponent(LightId lightId, const LightComponent& lightComponent)
{
    LightInstance* lightInstance = getLight(lightId);
    if (lightInstance == nullptr)
    {
        return false;
    }

    LightComponent sanitized = lightComponent;
    sanitizeLightComponent(sanitized);

    lightInstance->lightComponent = sanitized;
    return true;
}

LightId LightSystem::createLight(OwnerId ownerId, const LightComponent& lightComponent)
{
    if (m_owners.find(ownerId) == m_owners.end())
    {
        return 0;
    }

    LightComponent light = lightComponent;
    sanitizeLightComponent(light);

    const LightId newLightId = m_nextLightId++;
    m_lights.emplace(newLightId, LightInstance{ ownerId, light });
    return newLightId;
}

bool LightSystem::destroyLight(LightId lightId)
{
    return m_lights.erase(lightId) > 0;
}

LightInstance* LightSystem::getLight(LightId lightId)
{
    auto lightIterator = m_lights.find(lightId);
    if (lightIterator == m_lights.end())
    {
        return nullptr;
    }

    return &lightIterator->second;
}

const LightInstance* LightSystem::getLight(LightId lightId) const
{
    const auto lightIterator = m_lights.find(lightId);
    if (lightIterator == m_lights.end())
    {
        return nullptr;
    }

    return &lightIterator->second;
}

LightId LightSystem::createDirectionalLight(OwnerId ownerId, const LightCommon& common)
{
    LightComponent lightComponent{};
    lightComponent.common = common;
    lightComponent.type = LightType::DIRECTIONAL;
    lightComponent.parameters = LightParameters::makeDirectional();

    return createLight(ownerId, lightComponent);
}

LightId LightSystem::createPointLight(OwnerId ownerId, const LightCommon& common, const PointLightParameters& params)
{
    LightComponent lightComponent{};
    lightComponent.common = common;
    lightComponent.type = LightType::POINT;
    lightComponent.parameters.point = params;

    return createLight(ownerId, lightComponent);
}

LightId LightSystem::createSpotLight(OwnerId ownerId, const LightCommon& common, const SpotLightParameters& params)
{
    LightComponent lightComponent{};
    lightComponent.common = common;
    lightComponent.type = LightType::SPOT;
    lightComponent.parameters.spot = params;

    return createLight(ownerId, lightComponent);
}

void LightSystem::setAmbient(const Vector3& ambientColorValue, float ambientIntensityValue)
{
    m_ambientColor = ambientColorValue;
    m_ambientIntensity = std::max(0.0f, ambientIntensityValue);
}

Vector3 LightSystem::getAmbientColor() const
{
    return m_ambientColor;
}

float LightSystem::getAmbientIntensity() const
{
    return m_ambientIntensity;
}

PackedLightsGPU LightSystem::packForGPU() const
{
    PackedLightsGPU packedLights{};
    packedLights.ambientColor = m_ambientColor;
    packedLights.ambientIntensity = m_ambientIntensity;

    static constexpr float DEGREES_TO_RADIANS = PI / 180.0f;

    for (const auto& lightPair : m_lights)
    {
        const LightInstance& lightInstance = lightPair.second;
        const LightComponent& lightComponent = lightInstance.lightComponent;
        const LightCommon& common = lightComponent.common;

        if (!common.enabled)
        {
            continue;
        }

        const auto ownerIterator = m_owners.find(lightInstance.ownerId);
        if (ownerIterator == m_owners.end())
        {
            continue;
        }

        const ManualTransform& ownerTransform = ownerIterator->second;

        const Vector3 forward = safeNormalize(ownerTransform.forward, Vector3::Forward);

        switch (lightComponent.type)
        {
        case LightType::DIRECTIONAL:
        {
            GPUDirectionalLight gpuLight{};
            gpuLight.direction = forward;
            gpuLight.color = common.color;
            gpuLight.intensity = common.intensity;

            packedLights.directionalLights.push_back(gpuLight);
            break;
        }

        case LightType::POINT:
        {
            GPUPointLight gpuLight{};
            gpuLight.position = ownerTransform.position;
            gpuLight.radius = lightComponent.parameters.point.radius;
            gpuLight.color = common.color;
            gpuLight.intensity = common.intensity;

            packedLights.pointLights.push_back(gpuLight);
            break;
        }

        case LightType::SPOT:
        {
            const SpotLightParameters& spot = lightComponent.parameters.spot;

            GPUSpotLight gpuLight{};
            gpuLight.position = ownerTransform.position;
            gpuLight.direction = forward;
            gpuLight.radius = spot.radius;
            gpuLight.color = common.color;
            gpuLight.intensity = common.intensity;

            const float innerRadians = spot.innerAngleDegrees * DEGREES_TO_RADIANS;
            const float outerRadians = spot.outerAngleDegrees * DEGREES_TO_RADIANS;

            gpuLight.cosineInnerAngle = std::cos(innerRadians);
            gpuLight.cosineOuterAngle = std::cos(outerRadians);

            packedLights.spotLights.push_back(gpuLight);
            break;
        }

        default:
        {
            break;
        }
        }
    }

    return packedLights;
}


GPULightsConstantBuffer LightSystem::packForGPUConstantBuffer() const
{
    GPULightsConstantBuffer constantBuffer{};
    constantBuffer.ambientColor = m_ambientColor;
    constantBuffer.ambientIntensity = m_ambientIntensity;

    const PackedLightsGPU packedLights = packForGPU();

    constantBuffer.directionalCount = std::min<uint32_t>(
        static_cast<uint32_t>(packedLights.directionalLights.size()),
        LightDefaults::MAX_DIRECTIONAL_LIGHTS
    );

    constantBuffer.pointCount = std::min<uint32_t>(
        static_cast<uint32_t>(packedLights.pointLights.size()),
        LightDefaults::MAX_POINT_LIGHTS
    );

    constantBuffer.spotCount = std::min<uint32_t>(
        static_cast<uint32_t>(packedLights.spotLights.size()),
        LightDefaults::MAX_SPOT_LIGHTS
    );

    for (uint32_t i = 0; i < constantBuffer.directionalCount; ++i)
    {
        constantBuffer.directionalLights[i] = packedLights.directionalLights[i];
    }

    for (uint32_t i = 0; i < constantBuffer.pointCount; ++i)
    {
        constantBuffer.pointLights[i] = packedLights.pointLights[i];
    }

    for (uint32_t i = 0; i < constantBuffer.spotCount; ++i)
    {
        constantBuffer.spotLights[i] = packedLights.spotLights[i];
    }

    return constantBuffer;
}
