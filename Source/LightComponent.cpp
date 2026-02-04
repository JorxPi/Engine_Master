#include "Globals.h"
#include "LightComponent.h"

#include <algorithm>
#include <cmath>

namespace
{
    constexpr float SPOT_MAX_ANGLE_DEGREES = 89.9f;
    constexpr float SPOT_MIN_ANGLE_DELTA_DEGREES = 0.001f;

    static void sanitizeSpotAngles(float& innerAngleDegrees, float& outerAngleDegrees)
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
}

LightComponent::LightComponent()
{
    m_type = ComponentType::LIGHT;
}

const LightData& LightComponent::getData() const
{
    return m_data;
}

LightData& LightComponent::editData()
{
    return m_data;
}

void LightComponent::setTypeDirectional()
{
    m_data.type = LightType::DIRECTIONAL;
    m_data.parameters = LightParameters::makeDirectional();
    sanitize();
}

void LightComponent::setTypePoint(float radius)
{
    m_data.type = LightType::POINT;
    m_data.parameters = LightParameters::makePoint(radius);
    sanitize();
}

void LightComponent::setTypeSpot(float radius, float innerAngleDegrees, float outerAngleDegrees)
{
    m_data.type = LightType::SPOT;
    m_data.parameters = LightParameters::makeSpot(radius, innerAngleDegrees, outerAngleDegrees);
    sanitize();
}

void LightComponent::sanitize()
{
    m_data.common.intensity = std::max(0.0f, m_data.common.intensity);

    if (m_data.type == LightType::POINT)
    {
        m_data.parameters.point.radius = std::max(0.0f, m_data.parameters.point.radius);
        return;
    }

    if (m_data.type == LightType::SPOT)
    {
        m_data.parameters.spot.radius = std::max(0.0f, m_data.parameters.spot.radius);
        sanitizeSpotAngles(
            m_data.parameters.spot.innerAngleDegrees,
            m_data.parameters.spot.outerAngleDegrees);
        return;
    }
}
