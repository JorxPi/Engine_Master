#pragma once

#include "Lights.h"

class GameObject;

class LightSystem
{
public:
    GPULightsConstantBuffer packForGPUConstantBuffer(const std::vector<GameObject*>& objects) const;

    void setAmbient(const Vector3& ambientColorValue, float ambientIntensityValue);
    Vector3 getAmbientColor() const;
    float getAmbientIntensity() const;
private:
    Vector3 m_ambientColor = LightDefaults::DEFAULT_AMBIENT_COLOR;
    float m_ambientIntensity = LightDefaults::DEFAULT_AMBIENT_INTENSITY;
};
