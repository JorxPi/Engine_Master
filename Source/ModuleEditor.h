#pragma once

#include "Module.h"
#include "ImGuiPass.h"
#include "LogConsole.h"

class ModuleCameraEditor;
class ModulePipeline;

class ModuleEditor : public Module
{
public:
	ModuleEditor(HWND windowHandle);

	bool postInit() override;
	void preRender() override;
	void render() override;

	void logg(const char* format, ...);

	void drawDocSpace();

	// View
	void drawConsoleWindow();
	void drawImGuiDocWindow();
	void drawAboutWindow();

	// Options
	void drawConfigWindow();
	void drawAppInfo() const;
	void drawWindowOptions();
	void drawHardwareOptions() const;

	// Camera
	void drawCameraWindow(ModuleCameraEditor* camMod);

	// Texture
	void drawTextureSamplesWindow(ModulePipeline* pipe);
	void drawTextureGridWindow(ModulePipeline* pipe);
	//void drawTextureChangeWindow(ModulePipeline* pipe);

	//Model
	void drawGeometryViewerWindow(ModulePipeline* pipe, ModuleCameraEditor* cam);


private:
	HWND hWnd = nullptr;
	std::unique_ptr<ImGuiPass> imguiPass;

	LogConsole logConsole;

	bool showConsole = false;  
	bool showDemo = false;
	bool showConfig = false;
	bool showAbout = false;
	bool showCameraWindow = false;
	bool showTextureSamples = false;
	bool showTextureGrid = false;
	bool showTextureChange = false;
	bool showGeometryViewer = false;

	float gizmoDesiredPixels = 120.0f;
	float gizmoViewportH = 1.0f;

};
