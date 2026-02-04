#include "Globals.h"
#include "LightComponent.h"
#include <algorithm>
#include <cmath>

namespace
{
    constexpr float SPOT_MAX_ANGLE_DEGREES = 89.9f;
    constexpr float SPOT_MIN_DELTA = 0.001f;
}

static void sanitizeSpotAngles(float& innerDeg, float& outerDeg)
{
    innerDeg = std::clamp(innerDeg, 0.0f, SPOT_MAX_ANGLE_DEGREES);
    outerDeg = std::clamp(outerDeg, 0.0f, SPOT_MAX_ANGLE_DEGREES);

    if (innerDeg > outerDeg) std::swap(innerDeg, outerDeg);
    if (std::abs(innerDeg - outerDeg) < SPOT_MIN_DELTA)
        outerDeg = std::min(SPOT_MAX_ANGLE_DEGREES, innerDeg + SPOT_MIN_DELTA);
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

void LightComponent::setTypeSpot(float radius, float innerDeg, float outerDeg)
{
    m_data.type = LightType::SPOT;
    m_data.parameters = LightParameters::makeSpot(radius, innerDeg, outerDeg);
    sanitize();
}

void LightComponent::sanitize()
{
    m_data.common.intensity = std::max(0.0f, m_data.common.intensity);

    switch (m_data.type)
    {
    case LightType::POINT:
        m_data.parameters.point.radius = std::max(0.0f, m_data.parameters.point.radius);
        break;

    case LightType::SPOT:
        m_data.parameters.spot.radius = std::max(0.0f, m_data.parameters.spot.radius);
        sanitizeSpotAngles(m_data.parameters.spot.innerAngleDegrees,
            m_data.parameters.spot.outerAngleDegrees);
        break;

    case LightType::DIRECTIONAL:
    default:
        break;
    }
}
