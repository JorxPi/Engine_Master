#pragma once

#include "Lights.h"

class GameObject;

class LightSystem
{
public:
    void setAmbient(const Vector3& ambientColorValue, float ambientIntensityValue);
    Vector3 getAmbientColor() const;
    float getAmbientIntensity() const;

    GPULightsConstantBuffer packForGPUConstantBuffer(const std::vector<GameObject*>& objects) const;

private:
    Vector3 m_ambientColor = LightDefaults::DEFAULT_AMBIENT_COLOR;
    float m_ambientIntensity = LightDefaults::DEFAULT_AMBIENT_INTENSITY;
};
