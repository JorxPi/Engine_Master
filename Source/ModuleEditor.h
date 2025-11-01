#pragma once

#include "Module.h"
#include "ImGuiPass.h"

class ModuleEditor : public Module
{
public:
	ModuleEditor();

	bool postInit() override;
	void preRender() override;
	void render() override;
	void postRender() override;

private:
	std::unique_ptr<ImGuiPass> imguiPass;
};
