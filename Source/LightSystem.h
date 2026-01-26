#pragma once

#include "Lights.h"
#include <unordered_map>

class LightSystem
{
public:
    //Provisional while no GO
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

    LightId createDirectionalLight(
        OwnerId ownerId,
        const LightCommon& common = {},
        const DirectionalLightParameters& parameters = {});

    LightId createPointLight(
        OwnerId ownerId,
        const LightCommon& common,
        const PointLightParameters& parameters);

    LightId createSpotLight(
        OwnerId ownerId,
        const LightCommon& common,
        const SpotLightParameters& parameters);

    void setAmbient(const Vector3& ambientColorValue, float ambientIntensityValue);
    Vector3 getAmbientColor() const { return ambientColor; }
    float getAmbientIntensity() const { return ambientIntensity; }

    PackedLightsGPU packForGPU() const;
    GPULightsConstantBuffer packForGPUConstantBuffer() const;


private:
    static Vector3 safeNormalize(const Vector3& vectorToNormalize, const Vector3& fallbackValue);
    static void sanitizeSpotAngles(float& innerAngleDegrees, float& outerAngleDegrees);

private:
    OwnerId nextOwnerId = 1;
    LightId nextLightId = 1;

    std::unordered_map<OwnerId, ManualTransform> owners;
    std::unordered_map<LightId, LightInstance> lights;

    Vector3 ambientColor = Vector3(0.1f, 0.1f, 0.1f);
    float ambientIntensity = 1.0f;
};
