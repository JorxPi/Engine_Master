#include "Globals.h"
#include "LightSystem.h"
#include "GameObject.h"
#include "Transform.h"
#include "LightComponent.h"

#include <algorithm>
#include <cmath>

namespace
{
    static constexpr float PI = 3.14159265358979323846f;
    static constexpr float DEGREES_TO_RADIANS = PI / 180.0f;
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

GPULightsConstantBuffer LightSystem::packForGPUConstantBuffer(const std::vector<GameObject*>& objects) const
{
    GPULightsConstantBuffer cb{};
    cb.ambientColor = m_ambientColor;
    cb.ambientIntensity = m_ambientIntensity;

    // Temporary CPU vectors to clamp to MAX_* cleanly
    std::vector<GPUDirectionalLight> dir;
    std::vector<GPUPointLight>       pt;
    std::vector<GPUSpotLight>        sp;

    for (GameObject* go : objects)
    {
        if (!go) continue;

        LightComponent* lc = go->GetLightComponent();
        if (!lc) continue;

        LightData& data = lc->editData();
        lc->sanitize();

        const LightCommon& common = data.common;
        if (!common.enabled) continue;

        Transform* tr = go->GetTransform();
        if (!tr) continue;

        const Vector3 pos = *tr->getPosition();
        Vector3 forward = tr->getForward();
        forward.Normalize();

        switch (data.type)
        {
        case LightType::DIRECTIONAL:
        {
            GPUDirectionalLight g{};
            g.direction = forward;
            g.color = common.color;
            g.intensity = common.intensity;
            dir.push_back(g);
            break;
        }
        case LightType::POINT:
        {
            GPUPointLight g{};
            g.position = pos;
            g.radius = data.parameters.point.radius;
            g.color = common.color;
            g.intensity = common.intensity;
            pt.push_back(g);
            break;
        }
        case LightType::SPOT:
        {
            const SpotLightParameters& s = data.parameters.spot;

            GPUSpotLight g{};
            g.position = pos;
            g.direction = forward;
            g.radius = s.radius;
            g.color = common.color;
            g.intensity = common.intensity;

            g.cosineInnerAngle = std::cos(s.innerAngleDegrees * DEGREES_TO_RADIANS);
            g.cosineOuterAngle = std::cos(s.outerAngleDegrees * DEGREES_TO_RADIANS);

            sp.push_back(g);
            break;
        }
        default:
            break;
        }
    }

    cb.directionalCount = std::min<uint32_t>((uint32_t)dir.size(), LightDefaults::MAX_DIRECTIONAL_LIGHTS);
    cb.pointCount = std::min<uint32_t>((uint32_t)pt.size(), LightDefaults::MAX_POINT_LIGHTS);
    cb.spotCount = std::min<uint32_t>((uint32_t)sp.size(), LightDefaults::MAX_SPOT_LIGHTS);

    for (uint32_t i = 0; i < cb.directionalCount; ++i) cb.directionalLights[i] = dir[i];
    for (uint32_t i = 0; i < cb.pointCount; ++i)       cb.pointLights[i] = pt[i];
    for (uint32_t i = 0; i < cb.spotCount; ++i)        cb.spotLights[i] = sp[i];

    return cb;
}
