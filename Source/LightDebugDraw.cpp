#include "Globals.h"
#include "LightDebugDraw.h"
#include <algorithm>

static inline const float* asFloat3(const Vector3& vector) { return &vector.x; }

static Vector3 safeNormalize(const Vector3& vector, const Vector3& fallback)
{
    if (vector.LengthSquared() < 1e-8f) {
        return fallback;
    }
    Vector3 n = vector;
    n.Normalize(); 
    return n;
}

static void drawDirectional(const ManualTransform& transform, const LightComponent& lightComponent, bool depthEnabled)
{
    const Vector3 forwardDirection = safeNormalize(transform.forward, Vector3::Forward);

    const float length = 2.0f;
    const float headSize = 0.15f;
    Vector3 arrowEndPoint = transform.position + forwardDirection * length;
    const Vector3 color = lightComponent.common.color;

    dd::arrow(asFloat3(transform.position), asFloat3(arrowEndPoint), asFloat3(color), headSize, 0, depthEnabled);
}

static void drawPoint(const ManualTransform& transform, const LightComponent& lightComponent, bool depthEnabled)
{
    const float radius = std::max(0.0f, lightComponent.parameters.point.radius);
    const Vector3 color = lightComponent.common.color;

    dd::sphere(asFloat3(transform.position), asFloat3(color), radius, 0, depthEnabled);
}

static void drawSpot(const ManualTransform& transform, const LightComponent& lightComponent, bool depthEnabled)
{
    const float length = std::max(0.0f, lightComponent.parameters.spot.radius);
    float outerDegree = std::clamp(lightComponent.parameters.spot.outerAngleDegrees, 0.0f, 89.0f);
    float innerDegree = std::clamp(lightComponent.parameters.spot.innerAngleDegrees, 0.0f, outerDegree);

    const float outerRadians = XMConvertToRadians(outerDegree);
    const float innerRadians = XMConvertToRadians(innerDegree);

    Vector3 direction = safeNormalize(transform.forward, Vector3::Forward) * length;

    const float outerBaseRadius = std::tan(outerRadians) * length;
    const float innerBaseRadius = std::tan(innerRadians) * length;

    const Vector3 color = lightComponent.common.color;

    dd::cone(asFloat3(transform.position), asFloat3(direction), asFloat3(color), outerBaseRadius, 0.0f, 0, depthEnabled);
    dd::cone(asFloat3(transform.position), asFloat3(direction), asFloat3(color), innerBaseRadius, 0.0f, 0, depthEnabled);
}

namespace LightDebugDraw
{
    void drawLight(const LightSystem& lightSystem, LightId lightId, bool depthEnabled)
    {
        ManualTransform transform{};
        if (!lightSystem.getOwnerTransformFromLight(lightId, transform)) {
            return;
        }

        const LightInstance* instance = lightSystem.getLight(lightId);
        if (!instance) {
            return;
        }

        const LightComponent& lightComponent = instance->lightComponent;
        const LightCommon& common = lightComponent.common;
        if (!common.enabled) {
            return;
        }

        switch (lightComponent.type)
        {
        case LightType::DIRECTIONAL:
        {
            drawDirectional(transform, lightComponent, depthEnabled);
            break;
        }

        case LightType::POINT:
        {
            drawPoint(transform, lightComponent, depthEnabled);
            break;
        }

        case LightType::SPOT:
        {
            drawSpot(transform, lightComponent, depthEnabled);
            break;
        }

        default:
            break;
        }
    }
}
