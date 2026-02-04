#pragma once
#include "Lights.h"
#include "LightSystem.h"

namespace LightDebugDraw
{
    void drawLightWithoutDepth(const GameObject& gameObject);
    void drawLightWithDepth(const GameObject& gameObject);
}