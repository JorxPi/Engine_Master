#include "Globals.h"
#include "DebugDrawer.h"
#include <cmath>
#include <algorithm>

static constexpr float kPi = 3.14159265358979323846f;

Vector3 DebugDrawer::safeNormalize(const Vector3& vector, const Vector3& fallback)
{
    if (vector.LengthSquared() < 1e-8f)
        return fallback;

    Vector3 normalizedVector = vector;
    normalizedVector.Normalize();
    return normalizedVector;
}

Vector3 DebugDrawer::safeOrtho(const Vector3& normal)
{
    Vector3 normalizedNormal = safeNormalize(normal, Vector3::Forward);

    Vector3 referenceAxis =
        (fabsf(normalizedNormal.Dot(Vector3::Up)) > 0.99f) ? Vector3::Right : Vector3::Up;

    Vector3 rightVector = normalizedNormal.Cross(referenceAxis);
    if (rightVector.LengthSquared() < 1e-8f)
        rightVector = Vector3::Right;

    rightVector.Normalize();
    return rightVector;
}

void DebugDrawer::addArrow(DebugDrawData& output, const Vector3& position, const Vector3& direction, const Vector3& color) const
{
    Vector3 normalizedDirection = safeNormalize(direction, Vector3::Forward);

    output.lines.push_back({ position, position + normalizedDirection * directionalLength, color });

    Vector3 rightVector = safeOrtho(normalizedDirection);
    Vector3 upVector = rightVector.Cross(normalizedDirection);
    upVector.Normalize();

    float arrowHeadLength = directionalLength * 0.15f;
    Vector3 arrowTip = position + normalizedDirection * directionalLength;

    output.lines.push_back({ arrowTip, arrowTip - normalizedDirection * arrowHeadLength + rightVector * arrowHeadLength * 0.5f, color });
    output.lines.push_back({ arrowTip, arrowTip - normalizedDirection * arrowHeadLength - rightVector * arrowHeadLength * 0.5f, color });
    output.lines.push_back({ arrowTip, arrowTip - normalizedDirection * arrowHeadLength + upVector * arrowHeadLength * 0.5f, color });
    output.lines.push_back({ arrowTip, arrowTip - normalizedDirection * arrowHeadLength - upVector * arrowHeadLength * 0.5f, color });
}

void DebugDrawer::addWireCircle(DebugDrawData& output, const Vector3& center, const Vector3& axisNormal, float radius, int segments, const Vector3& color) const
{
    Vector3 normalizedAxis = safeNormalize(axisNormal, Vector3::Up);

    Vector3 rightVector = safeOrtho(normalizedAxis);
    Vector3 upVector = rightVector.Cross(normalizedAxis);
    upVector.Normalize();

    Vector3 previousPoint = center + rightVector * radius;

    for (int segmentIndex = 1; segmentIndex <= segments; ++segmentIndex)
    {
        float segmentFraction = (float)segmentIndex / (float)segments;
        float angleRadians = segmentFraction * (2.0f * kPi);

        Vector3 circlePoint =
            center + (rightVector * cosf(angleRadians) + upVector * sinf(angleRadians)) * radius;

        output.lines.push_back({ previousPoint, circlePoint, color });
        previousPoint = circlePoint;
    }
}

void DebugDrawer::addWireSphere(DebugDrawData& output, const Vector3& center, float radius, const Vector3& color) const
{
    radius = std::max(0.0f, radius);

    addWireCircle(output, center, Vector3::Up, radius, circleSegments, color);
    addWireCircle(output, center, Vector3::Right, radius, circleSegments, color);
    addWireCircle(output, center, Vector3::Forward, radius, circleSegments, color);
}

void DebugDrawer::addWireCone(DebugDrawData& output, const Vector3& apex, const Vector3& direction, float length, float outerAngleDegrees, const Vector3& color) const
{
    length = std::max(0.0f, length);
    if (length <= 1e-5f)
        return;

    Vector3 normalizedDirection = safeNormalize(direction, Vector3::Forward);

    float angleRadians = outerAngleDegrees * (kPi / 180.0f);
    float baseRadius = tanf(angleRadians) * length;

    Vector3 baseCenter = apex + normalizedDirection * length;

    Vector3 rightVector = safeOrtho(normalizedDirection);
    Vector3 upVector = rightVector.Cross(normalizedDirection);
    upVector.Normalize();

    Vector3 previousPoint = baseCenter + rightVector * baseRadius;

    for (int segmentIndex = 1; segmentIndex <= circleSegments; ++segmentIndex)
    {
        float segmentFraction = (float)segmentIndex / (float)circleSegments;
        float angle = segmentFraction * (2.0f * kPi);

        Vector3 circlePoint =
            baseCenter + (rightVector * cosf(angle) + upVector * sinf(angle)) * baseRadius;

        output.lines.push_back({ previousPoint, circlePoint, color });
        output.lines.push_back({ apex, circlePoint, color });

        previousPoint = circlePoint;
    }

    output.lines.push_back({ apex, baseCenter, color });
}

void DebugDrawer::addDirectionalLight(DebugDrawData& output, const LightSystem& lightSystem, OwnerId ownerId, LightId lightId) const
{
    ManualTransform ownerTransform{};
    if (!lightSystem.getOwnerTransform(ownerId, ownerTransform))
        return;

    const LightInstance* lightInstance = lightSystem.getLight(lightId);
    if (!lightInstance)
        return;

    if (!lightInstance->lightComponent.common.enabled ||
        lightInstance->lightComponent.type != LightType::Directional)
        return;

    addArrow(output, ownerTransform.position, ownerTransform.forward, lightInstance->lightComponent.common.color);
}

void DebugDrawer::addPointLight(DebugDrawData& output, const LightSystem& lightSystem, OwnerId ownerId, LightId lightId) const
{
    ManualTransform ownerTransform{};
    if (!lightSystem.getOwnerTransform(ownerId, ownerTransform))
        return;

    const LightInstance* lightInstance = lightSystem.getLight(lightId);
    if (!lightInstance)
        return;

    if (!lightInstance->lightComponent.common.enabled ||
        lightInstance->lightComponent.type != LightType::Point)
        return;

    const float radius = lightInstance->lightComponent.parameters.point.radius;
    addWireSphere(output, ownerTransform.position, radius, lightInstance->lightComponent.common.color);
}

void DebugDrawer::addSpotLight(DebugDrawData& output, const LightSystem& lightSystem, OwnerId ownerId, LightId lightId) const
{
    ManualTransform ownerTransform{};
    if (!lightSystem.getOwnerTransform(ownerId, ownerTransform))
        return;

    const LightInstance* lightInstance = lightSystem.getLight(lightId);
    if (!lightInstance)
        return;

    if (!lightInstance->lightComponent.common.enabled ||
        lightInstance->lightComponent.type != LightType::Spot)
        return;

    const float length = lightInstance->lightComponent.parameters.spot.radius;
    const float outerAngleDegrees = lightInstance->lightComponent.parameters.spot.outerAngleDegrees;

    addWireCone(output, ownerTransform.position, ownerTransform.forward, length, outerAngleDegrees, lightInstance->lightComponent.common.color);
}
