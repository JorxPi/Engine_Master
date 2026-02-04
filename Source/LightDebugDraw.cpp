#include "Globals.h"
#include "LightDebugDraw.h"
#include "GameObject.h"
#include "Transform.h"
#include "LightComponent.h"
#include <algorithm>

static inline const float* asFloat3(const Vector3& v) { return &v.x; }

namespace LightDebugDraw
{
    void drawLight(const GameObject& go, bool depthEnabled)
    {
        const LightComponent* lc = go.GetLightComponent();
        if (!lc) return;

        const LightData& data = lc->getData();
        if (!data.common.enabled) return;

        const Transform* tr = go.GetTransform();
        if (!tr) return;

        const Vector3 pos = *tr->getPosition();
        const Vector3 fwd = tr->getForward();
        const Vector3 color = data.common.color;

        switch (data.type)
        {
        case LightType::DIRECTIONAL:
        {
            Vector3 end = pos + fwd * 2.0f;
            dd::arrow(asFloat3(pos), asFloat3(end), asFloat3(color), 0.15f, 0, depthEnabled);
            break;
        }
        case LightType::POINT:
        {
            dd::sphere(asFloat3(pos), asFloat3(color), data.parameters.point.radius, 0, depthEnabled);
            break;
        }
        case LightType::SPOT:
        {
            float length = data.parameters.spot.radius;
            float outerRad = XMConvertToRadians(std::clamp(data.parameters.spot.outerAngleDegrees, 0.0f, 89.0f));
            float innerRad = XMConvertToRadians(std::clamp(data.parameters.spot.innerAngleDegrees, 0.0f, 89.0f));

            Vector3 dir = fwd * length;
            float outerBase = std::tan(outerRad) * length;
            float innerBase = std::tan(innerRad) * length;

            dd::cone(asFloat3(pos), asFloat3(dir), asFloat3(color), outerBase, 0.0f, 0, depthEnabled);
            dd::cone(asFloat3(pos), asFloat3(dir), asFloat3(color), innerBase, 0.0f, 0, depthEnabled);
            break;
        }
        default:
            break;
        }
    }
}
