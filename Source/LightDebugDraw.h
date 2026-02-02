#pragma once
#include "Lights.h"
#include "LightSystem.h"

namespace LightDebugDraw
{
    void drawLight(const LightSystem& lightSystem, LightId lightId, bool depthEnabled = true);
}