#pragma once
#include "Component.h"
#include "Lights.h"

class LightComponent final : public Component
{
public:
    LightComponent()
    {
        m_type = ComponentType::LIGHT;
    }

    const LightData& getData() const { return m_data; }

    LightData& editData() { return m_data; }

    void setTypeDirectional();

    void setTypePoint(float radius);

    void setTypeSpot(float radius, float innerDeg, float outerDeg);

    void sanitize();

private:
    LightData m_data{};
};
