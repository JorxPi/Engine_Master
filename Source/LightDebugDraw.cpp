#include "Globals.h"
#include "LightDebugDraw.h"

#include "GameObject.h"
#include "Transform.h"
#include "LightComponent.h"

#include <algorithm>

namespace
{
    constexpr float DIRECTIONAL_ARROW_LENGTH = 2.0f;
    constexpr float DIRECTIONAL_ARROW_HEAD_LENGTH = 0.15f;
    constexpr float SPOT_DEBUG_MAX_ANGLE_DEGREES = 89.0f;

    static inline const float* asFloat3(const Vector3& vector)
    {
        return &vector.x;
    }

    static void drawLight(const GameObject& gameObject, bool depthEnabled)
    {
        const LightComponent* lightComponent = gameObject.GetLightComponent();
        if (lightComponent == nullptr)
        {
            return;
        }

        const LightData& lightData = lightComponent->getData();
        if (!lightData.common.enabled)
        {
            return;
        }

        const Transform* transform = gameObject.GetTransform();
        if (transform == nullptr)
        {
            return;
        }

        const Vector3 position = *transform->getPosition();
        const Vector3 forward = transform->getForward();
        const Vector3 color = lightData.common.color;

        switch (lightData.type)
        {
        case LightType::DIRECTIONAL:
        {
            const Vector3 endPosition = position + forward * DIRECTIONAL_ARROW_LENGTH;
            dd::arrow(asFloat3(position), asFloat3(endPosition), asFloat3(color), DIRECTIONAL_ARROW_HEAD_LENGTH, 0, depthEnabled);
            break;
        }

        case LightType::POINT:
        {
            dd::sphere(asFloat3(position), asFloat3(color), lightData.parameters.point.radius, 0, depthEnabled);
            break;
        }

        case LightType::SPOT:
        {
            const float length = lightData.parameters.spot.radius;

            const float clampedOuterAngleDegrees = std::clamp(lightData.parameters.spot.outerAngleDegrees, 0.0f, SPOT_DEBUG_MAX_ANGLE_DEGREES);
            const float clampedInnerAngleDegrees = std::clamp(lightData.parameters.spot.innerAngleDegrees, 0.0f, SPOT_DEBUG_MAX_ANGLE_DEGREES);

            const float outerAngleRadians = XMConvertToRadians(clampedOuterAngleDegrees);
            const float innerAngleRadians = XMConvertToRadians(clampedInnerAngleDegrees);

            const Vector3 coneDirection = forward * length;

            const float outerBaseRadius = std::tan(outerAngleRadians) * length;
            const float innerBaseRadius = std::tan(innerAngleRadians) * length;

            dd::cone(asFloat3(position), asFloat3(coneDirection), asFloat3(color), outerBaseRadius, 0.0f, 0, depthEnabled);
            dd::cone(asFloat3(position), asFloat3(coneDirection), asFloat3(color), innerBaseRadius, 0.0f, 0, depthEnabled);
            break;
        }

        default:
        {
            break;
        }
        }
    }
}

namespace LightDebugDraw
{
    void drawLightWithoutDepth(const GameObject& gameObject)
    {
        drawLight(gameObject, false);
    }

    void drawLightWithDepth(const GameObject& gameObject)
    {
        drawLight(gameObject, true);
    }
}
