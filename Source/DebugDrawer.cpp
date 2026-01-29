#include "Globals.h"
#include "DebugDrawer.h"

#include <algorithm>
#include <cmath>

namespace
{
    static constexpr float PI = 3.14159265358979323846f;
    static constexpr float NORMALIZE_EPSILON = 1e-8f;

    static constexpr float MIN_CONE_LENGTH = 1e-5f;

    static constexpr float ORTHO_PARALLEL_DOT_THRESHOLD = 0.99f;

    static constexpr float ARROW_HEAD_LENGTH_FACTOR = 0.15f;
    static constexpr float ARROW_HEAD_SIDE_FACTOR = 0.5f;

    float degreesToRadians(float degrees)
    {
        static constexpr float DEGREES_TO_RADIANS_FACTOR = PI / 180.0f;
        return degrees * DEGREES_TO_RADIANS_FACTOR;
    }
}

Vector3 DebugDrawer::safeNormalize(const Vector3& vectorToNormalize, const Vector3& fallbackValue)
{
    if (vectorToNormalize.LengthSquared() < NORMALIZE_EPSILON)
    {
        return fallbackValue;
    }

    Vector3 normalizedVector = vectorToNormalize;
    normalizedVector.Normalize();
    return normalizedVector;
}

Vector3 DebugDrawer::safeOrtho(const Vector3& normal)
{
    const Vector3 normalizedNormal = safeNormalize(normal, Vector3::Forward);

    const float upDot = std::abs(normalizedNormal.Dot(Vector3::Up));
    const Vector3 referenceAxis = (upDot > ORTHO_PARALLEL_DOT_THRESHOLD) ? Vector3::Right : Vector3::Up;

    Vector3 rightVector = normalizedNormal.Cross(referenceAxis);
    if (rightVector.LengthSquared() < NORMALIZE_EPSILON)
    {
        rightVector = Vector3::Right;
    }

    rightVector.Normalize();
    return rightVector;
}

void DebugDrawer::addArrow(DebugDrawData& output, const ArrowParams& params) const
{
    const Vector3 normalizedDirection = safeNormalize(params.direction, Vector3::Forward);

    output.lines.push_back({ params.position, params.position + normalizedDirection * params.length, params.color });

    Vector3 rightVector = safeOrtho(normalizedDirection);
    Vector3 upVector = rightVector.Cross(normalizedDirection);
    upVector.Normalize();

    const float arrowHeadLength = params.length * ARROW_HEAD_LENGTH_FACTOR;
    const Vector3 arrowTip = params.position + normalizedDirection * params.length;

    output.lines.push_back({ arrowTip, arrowTip - normalizedDirection * arrowHeadLength + rightVector * arrowHeadLength * ARROW_HEAD_SIDE_FACTOR, params.color });
    output.lines.push_back({ arrowTip, arrowTip - normalizedDirection * arrowHeadLength - rightVector * arrowHeadLength * ARROW_HEAD_SIDE_FACTOR, params.color });
    output.lines.push_back({ arrowTip, arrowTip - normalizedDirection * arrowHeadLength + upVector * arrowHeadLength * ARROW_HEAD_SIDE_FACTOR, params.color });
    output.lines.push_back({ arrowTip, arrowTip - normalizedDirection * arrowHeadLength - upVector * arrowHeadLength * ARROW_HEAD_SIDE_FACTOR, params.color });
}

void DebugDrawer::addWireCircle(DebugDrawData& output, const WireCircleParams& params) const
{
    const float radius = std::max(0.0f, params.radius);
    const int segments = std::max(3, params.segments);

    const Vector3 normalizedAxis = safeNormalize(params.axisNormal, Vector3::Up);

    Vector3 rightVector = safeOrtho(normalizedAxis);
    Vector3 upVector = rightVector.Cross(normalizedAxis);
    upVector.Normalize();

    Vector3 previousPoint = params.center + rightVector * radius;

    for (int segmentIndex = 1; segmentIndex <= segments; ++segmentIndex)
    {
        const float segmentFraction = static_cast<float>(segmentIndex) / static_cast<float>(segments);
        const float angleRadians = segmentFraction * (2.0f * PI);

        const Vector3 circlePoint = params.center + (rightVector * std::cos(angleRadians) + upVector * std::sin(angleRadians)) * radius;

        output.lines.push_back({ previousPoint, circlePoint, params.color });
        previousPoint = circlePoint;
    }
}

void DebugDrawer::addWireSphere(DebugDrawData& output, const WireSphereParams& params) const
{
    const float radius = std::max(0.0f, params.radius);
    const int segments = std::max(3, params.segments);

    WireCircleParams circleParams{};
    circleParams.center = params.center;
    circleParams.radius = radius;
    circleParams.segments = segments;
    circleParams.color = params.color;

    circleParams.axisNormal = Vector3::Up;
    addWireCircle(output, circleParams);

    circleParams.axisNormal = Vector3::Right;
    addWireCircle(output, circleParams);

    circleParams.axisNormal = Vector3::Forward;
    addWireCircle(output, circleParams);
}

void DebugDrawer::addWireCone(DebugDrawData& output, const WireConeParams& params) const
{
    const float length = std::max(0.0f, params.length);
    if (length <= MIN_CONE_LENGTH)
    {
        return;
    }

    const int segments = std::max(3, params.segments);

    const Vector3 normalizedDirection = safeNormalize(params.direction, Vector3::Forward);

    const float angleRadians = degreesToRadians(params.outerAngleDegrees);
    const float baseRadius = std::tan(angleRadians) * length;

    const Vector3 baseCenter = params.apex + normalizedDirection * length;

    Vector3 rightVector = safeOrtho(normalizedDirection);
    Vector3 upVector = rightVector.Cross(normalizedDirection);
    upVector.Normalize();

    Vector3 previousPoint = baseCenter + rightVector * baseRadius;

    for (int segmentIndex = 1; segmentIndex <= segments; ++segmentIndex)
    {
        const float segmentFraction = static_cast<float>(segmentIndex) / static_cast<float>(segments);
        const float angle = segmentFraction * (2.0f * PI);

        const Vector3 circlePoint = baseCenter + (rightVector * std::cos(angle) + upVector * std::sin(angle)) * baseRadius;

        output.lines.push_back({ previousPoint, circlePoint, params.color });
        output.lines.push_back({ params.apex, circlePoint, params.color });

        previousPoint = circlePoint;
    }

    output.lines.push_back({ params.apex, baseCenter, params.color });
}

void DebugDrawer::addDirectionalLight(DebugDrawData& output, const LightSystem& lightSystem, LightId lightId) const
{
    ManualTransform ownerTransform{};
    if (!lightSystem.getOwnerTransformFromLight(lightId, ownerTransform))
    {
        return;
    }

    const LightInstance* lightInstance = lightSystem.getLight(lightId);
    if (lightInstance == nullptr)
    {
        return;
    }

    if (!lightInstance->lightComponent.common.enabled)
    {
        return;
    }

    if (lightInstance->lightComponent.type != LightType::DIRECTIONAL)
    {
        return;
    }

    ArrowParams arrowParams{};
    arrowParams.position = ownerTransform.position;
    arrowParams.direction = ownerTransform.forward;
    arrowParams.color = lightInstance->lightComponent.common.color;
    arrowParams.length = m_directionalLength;

    addArrow(output, arrowParams);
}

void DebugDrawer::addPointLight(DebugDrawData& output, const LightSystem& lightSystem, LightId lightId) const
{
    ManualTransform ownerTransform{};
    if (!lightSystem.getOwnerTransformFromLight(lightId, ownerTransform))
    {
        return;
    }

    const LightInstance* lightInstance = lightSystem.getLight(lightId);
    if (lightInstance == nullptr)
    {
        return;
    }

    if (!lightInstance->lightComponent.common.enabled)
    {
        return;
    }

    if (lightInstance->lightComponent.type != LightType::POINT)
    {
        return;
    }

    WireSphereParams sphereParams{};
    sphereParams.center = ownerTransform.position;
    sphereParams.radius = lightInstance->lightComponent.parameters.point.radius;
    sphereParams.color = lightInstance->lightComponent.common.color;
    sphereParams.segments = m_circleSegments;

    addWireSphere(output, sphereParams);
}

void DebugDrawer::addSpotLight(DebugDrawData& output, const LightSystem& lightSystem, LightId lightId) const
{
    ManualTransform ownerTransform{};
    if (!lightSystem.getOwnerTransformFromLight(lightId, ownerTransform))
    {
        return;
    }

    const LightInstance* lightInstance = lightSystem.getLight(lightId);
    if (lightInstance == nullptr)
    {
        return;
    }

    if (!lightInstance->lightComponent.common.enabled)
    {
        return;
    }

    if (lightInstance->lightComponent.type != LightType::SPOT)
    {
        return;
    }

    WireConeParams coneParams{};
    coneParams.apex = ownerTransform.position;
    coneParams.direction = ownerTransform.forward;
    coneParams.length = lightInstance->lightComponent.parameters.spot.radius;
    coneParams.outerAngleDegrees = lightInstance->lightComponent.parameters.spot.outerAngleDegrees;
    coneParams.color = lightInstance->lightComponent.common.color;
    coneParams.segments = m_circleSegments;

    addWireCone(output, coneParams);
}
