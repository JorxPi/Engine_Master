#pragma once

#include "Component.h"
#include "Lights.h"

class LightComponent final : public Component
{
public:
    LightComponent();

    const LightData& getData() const;
    LightData& editData();

    void setTypeDirectional();
    void setTypePoint(float radius);
    void setTypeSpot(float radius, float innerAngleDegrees, float outerAngleDegrees);

    void sanitize();

private:
    LightData m_data{};
};
