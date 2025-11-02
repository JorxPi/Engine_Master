#pragma once

#include "Module.h"
#include "ImGuiPass.h"
#include "LogConsole.h"

class ModuleEditor : public Module
{
public:
	ModuleEditor(HWND windowHandle);

	bool postInit() override;
	void preRender() override;
	void render() override;
	void postRender() override;

	void log(const char* format, ...);

	void drawAppInfo();
	void drawWindowOptions();
	void drawHardwareOptions();

private:
	HWND hWnd = nullptr;
	std::unique_ptr<ImGuiPass> imguiPass;

	LogConsole logConsole;

	bool showConsole = false;  
	bool showDemo = false;
	bool showConfig = false;

};
