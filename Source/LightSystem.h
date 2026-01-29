#pragma once

#include "Lights.h"

class LightSystem
{
public:
    // Provisional while there is no GameObject system
    OwnerId createOwner(const ManualTransform& initialTransform = {});
    bool destroyOwner(OwnerId ownerId);

    bool setOwnerTransform(OwnerId ownerId, const ManualTransform& newTransform);
    bool getOwnerTransform(OwnerId ownerId, ManualTransform& outputTransform) const;

    bool setOwnerPosition(OwnerId ownerId, const Vector3& newPosition);
    bool setOwnerForward(OwnerId ownerId, const Vector3& newForward);

    LightId createLight(OwnerId ownerId, const LightComponent& lightComponent);
    bool destroyLight(LightId lightId);

    LightInstance* getLight(LightId lightId);
    const LightInstance* getLight(LightId lightId) const;

    bool getLightComponent(LightId lightId, LightComponent& outputComponent) const;
    bool setLightComponent(LightId lightId, const LightComponent& lightComponent);

    LightId createDirectionalLight(OwnerId ownerId, const LightCommon& common = {});
    LightId createPointLight(OwnerId ownerId, const LightCommon& common, const PointLightParameters& params);
    LightId createSpotLight(OwnerId ownerId, const LightCommon& common, const SpotLightParameters& params);

    OwnerId getOwnerIdFromLight(LightId lightId) const;
    bool getOwnerTransformFromLight(LightId lightId, ManualTransform& outputTransform) const;

    bool setOwnerTransformFromLight(LightId lightId, const ManualTransform& newTransform);
    bool setOwnerPositionFromLight(LightId lightId, const Vector3& newPosition);
    bool setOwnerForwardFromLight(LightId lightId, const Vector3& newForward);

    void setAmbient(const Vector3& ambientColorValue, float ambientIntensityValue);
    Vector3 getAmbientColor() const;
    float getAmbientIntensity() const;

    PackedLightsGPU packForGPU() const;
    GPULightsConstantBuffer packForGPUConstantBuffer() const;

private:
    static Vector3 safeNormalize(const Vector3& vectorToNormalize, const Vector3& fallbackValue);
    static void sanitizeSpotAngles(float& innerAngleDegrees, float& outerAngleDegrees);
    static void sanitizeManualTransform(ManualTransform& transform);
    static void sanitizeLightComponent(LightComponent& lightComponent);

private:
    OwnerId m_nextOwnerId = 1;
    LightId m_nextLightId = 1;

    std::unordered_map<OwnerId, ManualTransform> m_owners;
    std::unordered_map<LightId, LightInstance> m_lights;

    Vector3 m_ambientColor = LightDefaults::DEFAULT_AMBIENT_COLOR;
    float m_ambientIntensity = LightDefaults::DEFAULT_AMBIENT_INTENSITY;
};
