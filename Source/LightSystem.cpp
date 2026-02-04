#include "Globals.h"
#include "LightSystem.h"

#include "GameObject.h"
#include "Transform.h"
#include "LightComponent.h"

#include <algorithm>
#include <cmath>

GPULightsConstantBuffer LightSystem::packForGPUConstantBuffer(const std::vector<GameObject*>& objects) const
{
    GPULightsConstantBuffer constantBuffer{};
    constantBuffer.ambientColor = m_ambientColor;
    constantBuffer.ambientIntensity = m_ambientIntensity;

    std::vector<GPUDirectionalLight> directionalLights;
    std::vector<GPUPointLight> pointLights;
    std::vector<GPUSpotLight> spotLights;

    for (GameObject* gameObject : objects)
    {
        if (gameObject == nullptr)
        {
            continue;
        }

        const LightComponent* lightComponent = gameObject->GetLightComponent();
        if (lightComponent == nullptr)
        {
            continue;
        }

        const LightData& lightData = lightComponent->getData();
        const LightCommon& common = lightData.common;

        if (!common.enabled)
        {
            continue;
        }

        const Transform* transform = gameObject->GetTransform();
        if (transform == nullptr)
        {
            continue;
        }

        const Vector3 position = *transform->getPosition();
        Vector3 forward = transform->getForward();
        forward.Normalize();

        switch (lightData.type)
        {
        case LightType::DIRECTIONAL:
        {
            GPUDirectionalLight gpuLight{};
            gpuLight.direction = forward;
            gpuLight.color = common.color;
            gpuLight.intensity = common.intensity;
            directionalLights.push_back(gpuLight);
            break;
        }

        case LightType::POINT:
        {
            GPUPointLight gpuLight{};
            gpuLight.position = position;
            gpuLight.radius = lightData.parameters.point.radius;
            gpuLight.color = common.color;
            gpuLight.intensity = common.intensity;
            pointLights.push_back(gpuLight);
            break;
        }

        case LightType::SPOT:
        {
            const SpotLightParameters& spotParameters = lightData.parameters.spot;

            GPUSpotLight gpuLight{};
            gpuLight.position = position;
            gpuLight.direction = forward;
            gpuLight.radius = spotParameters.radius;
            gpuLight.color = common.color;
            gpuLight.intensity = common.intensity;

            gpuLight.cosineInnerAngle = std::cos(XMConvertToRadians(spotParameters.innerAngleDegrees));
            gpuLight.cosineOuterAngle = std::cos(XMConvertToRadians(spotParameters.outerAngleDegrees));

            spotLights.push_back(gpuLight);
            break;
        }

        default:
        {
            break;
        }
        }
    }

    constantBuffer.directionalCount = std::min<uint32_t>(
        static_cast<uint32_t>(directionalLights.size()),
        LightDefaults::MAX_DIRECTIONAL_LIGHTS);

    constantBuffer.pointCount = std::min<uint32_t>(
        static_cast<uint32_t>(pointLights.size()),
        LightDefaults::MAX_POINT_LIGHTS);

    constantBuffer.spotCount = std::min<uint32_t>(
        static_cast<uint32_t>(spotLights.size()),
        LightDefaults::MAX_SPOT_LIGHTS);

    for (uint32_t i = 0; i < constantBuffer.directionalCount; ++i)
    {
        constantBuffer.directionalLights[i] = directionalLights[i];
    }

    for (uint32_t i = 0; i < constantBuffer.pointCount; ++i)
    {
        constantBuffer.pointLights[i] = pointLights[i];
    }

    for (uint32_t i = 0; i < constantBuffer.spotCount; ++i)
    {
        constantBuffer.spotLights[i] = spotLights[i];
    }

    return constantBuffer;
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
