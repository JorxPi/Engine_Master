#pragma once

#include "Module.h"
#include "ImGuiPass.h"
#include "LogConsole.h"
#include <imgui.h>
#include "ImGuizmo.h"

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
	void drawTextureGridWindow(ModulePipeline* pipe);

	// Camera
	void drawCameraWindow(ModuleCameraEditor* camMod);

	//Model
	void drawGeometryViewerWindow(ModulePipeline* pipe, ModuleCameraEditor* cam);
	void drawPhongControlsWindow(ModulePipeline* pipe, ModuleCameraEditor* cam);

	void focusOnModel(ModulePipeline* pipe, ModuleCameraEditor* cam);

	//ImGuizmo
	void updateGizmoHotkeys();
	void drawGizmo(ModulePipeline* pipe, ModuleCameraEditor* cam);
	void updateGizmoSelection(ModulePipeline* pipe);

	//Scene
	void drawSceneWindow(ModulePipeline* pipe, ModuleCameraEditor* cam);



private:
	HWND hWnd = nullptr;
	std::unique_ptr<ImGuiPass> imguiPass;

	LogConsole logConsole;

	bool showConsole = false;  
	bool showDemo = false;
	bool showConfig = false;
	bool showAbout = false;
	bool showCameraWindow = false;
	bool showTextureGrid = false;
	bool showGeometryViewer = false;
	bool showPhongControls = false;

	//ImGuizmo
	ImGuizmo::OPERATION gizmoOp = ImGuizmo::TRANSLATE;
	ImGuizmo::MODE gizmoMode = ImGuizmo::LOCAL;
	bool showGizmo = true;
	bool hasSelection = true;

	//Scene
	ImVec2 sceneCanvasSize = ImVec2(0, 0);
	ImVec2 sceneCursorScreenPos = ImVec2(0, 0);
	bool sceneFocused = false;
	bool sceneHovered = false;
	ImDrawList* sceneDrawList = nullptr;
};
