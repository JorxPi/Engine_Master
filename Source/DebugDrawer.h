#pragma once

#include "Lights.h"
#include "LightSystem.h"

#include <vector>

struct DebugLine
{
    Vector3 start{};
    Vector3 end{};
    Vector3 color = Vector3::One;
};

struct DebugDrawData
{
    std::vector<DebugLine> lines;
};

class DebugDrawer
{
public:
    void addDirectionalLight(DebugDrawData& output, const LightSystem& lightSystem, LightId lightId) const;
    void addPointLight(DebugDrawData& output, const LightSystem& lightSystem, LightId lightId) const;
    void addSpotLight(DebugDrawData& output, const LightSystem& lightSystem, LightId lightId) const;

private:
    struct ArrowParams
    {
        Vector3 position{};
        Vector3 direction{};
        Vector3 color = Vector3::One;
        float length = 0.0f;
    };

    struct WireCircleParams
    {
        Vector3 center{};
        Vector3 axisNormal{};
        Vector3 color = Vector3::One;
        float radius = 0.0f;
        int segments = 0;
    };

    struct WireConeParams
    {
        Vector3 apex{};
        Vector3 direction{};
        Vector3 color = Vector3::One;
        float length = 0.0f;
        float outerAngleDegrees = 0.0f;
        int segments = 0;
    };

    struct WireSphereParams
    {
        Vector3 center{};
        Vector3 color = Vector3::One;
        float radius = 0.0f;
        int segments = 0;
    };

    void addArrow(DebugDrawData& output, const ArrowParams& params) const;
    void addWireCircle(DebugDrawData& output, const WireCircleParams& params) const;
    void addWireCone(DebugDrawData& output, const WireConeParams& params) const;
    void addWireSphere(DebugDrawData& output, const WireSphereParams& params) const;

    static Vector3 safeNormalize(const Vector3& vectorToNormalize, const Vector3& fallbackValue);
    static Vector3 safeOrtho(const Vector3& normal);
};
