#pragma once

#include "Module.h"
#include "ImGuiPass.h"
#include "LogConsole.h"
#include <imgui.h>
#include "ImGuizmo.h"

class ModuleCameraEditor;
class ModulePipeline;

enum class GizmoTarget
{
	None,
	Model,
	DirectionalLight,
	PointLight,
	SpotLight
};

class ModuleEditor : public Module
{
public:
	ModuleEditor(HWND windowHandle);

	bool postInit() override;
	void preRender() override;
	void render() override;

	void logg(const char* format, ...);

	void drawDocSpace();
	void buildDefaultLayout(ImGuiID dockspaceId);

	// View
	void drawConsoleWindow();
	void drawImGuiDocWindow();
	void drawAboutWindow();

	// Options
	void drawConfigWindow();
	void drawAppInfo();
	void drawWindowOptions();
	void drawHardwareOptions() const;
	void drawTextureGridWindow(ModulePipeline* pipe);

	// Camera
	void drawCameraWindow(ModuleCameraEditor* camMod);

	//Model
	void drawGeometryViewerWindow(ModulePipeline* pipe, ModuleCameraEditor* cam);
	void drawPhongControlsWindow(ModulePipeline* pipe, ModuleCameraEditor* cam);
	void drawLightsWindow(ModulePipeline* pipe);

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
	bool dockBuilt = false;
	bool requestResetLayout = false;

	LogConsole logConsole;

	bool showConsole = true;  
	bool showDemo = false;
	bool showConfig = false;
	bool showApplication = true;
	bool showAbout = false;
	bool showCameraWindow = false;
	bool showTextureGrid = false;
	bool showGeometryViewer = true;
	bool showPhongControls = true;
	bool showLightsWindow = true;

	//ImGuizmo
	ImGuizmo::OPERATION gizmoOp = ImGuizmo::TRANSLATE;
	ImGuizmo::MODE gizmoMode = ImGuizmo::LOCAL;
	bool showGizmo = true;
	bool hasSelection = true;
	GizmoTarget gizmoTarget = GizmoTarget::Model;

	//Scene
	ImVec2 sceneCanvasSize = ImVec2(0, 0);
	ImVec2 sceneCursorScreenPos = ImVec2(0, 0);
	bool sceneFocused = false;
	bool sceneHovered = false;
	ImDrawList* sceneDrawList = nullptr;
};
