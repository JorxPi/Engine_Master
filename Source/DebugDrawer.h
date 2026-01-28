#pragma once

#include "Lights.h"
#include "LightSystem.h"
#include <vector>

struct DebugLine
{
    Vector3 start;
    Vector3 end;
    Vector3 color = Vector3::One;
};

struct DebugDrawData
{
    std::vector<DebugLine> lines;
};

class DebugDrawer
{
public:
    float directionalLength = 2.0f;
    int circleSegments = 24;

    void addArrow(DebugDrawData& output, const Vector3& position, const Vector3& direction, const Vector3& color) const;

    void addWireSphere(DebugDrawData& output, const Vector3& center, float radius, const Vector3& color) const;

    void addWireCone(DebugDrawData& output, const Vector3& apex, const Vector3& direction, float length, float outerAngleDegrees, const Vector3& color) const;

    void addWireCircle(DebugDrawData& output, const Vector3& center, const Vector3& axisNormal, float radius, int segments, const Vector3& color) const;

    void addDirectionalLight(DebugDrawData& output, const LightSystem& lightSystem, OwnerId ownerId, LightId lightId) const;

    void addPointLight(DebugDrawData& output, const LightSystem& lightSystem, OwnerId ownerId, LightId lightId) const;

    void addSpotLight(DebugDrawData& output, const LightSystem& lightSystem, OwnerId ownerId, LightId lightId) const;

private:
    static Vector3 safeNormalize(const Vector3& vector, const Vector3& fallback);
    static Vector3 safeOrtho(const Vector3& normal);
};
