#include "Globals.h"
#include "Application.h"
#include "ModuleEditor.h"
#include "ModuleD3D12.h"
#include "imgui.h"

ModuleEditor::ModuleEditor(){

}

bool ModuleEditor::postInit() {
	auto ModRender = app->getModule<ModuleD3D12>();
	if (ModRender) {imguiPass = std::make_unique<ImGuiPass>(ModRender->getDevice(), ModRender->getWindowHandle());}
	return true;
}

void ModuleEditor::preRender() {
    imguiPass->startFrame();

    static bool showDemo = true;
    if (showDemo)
        ImGui::ShowDemoWindow(&showDemo);

    /*ImGui::Begin("Editor");
    ImGui::Text("Hello, Editor!");
    ImGui::Checkbox("Show ImGui Demo", &showDemo);
    ImGui::End();*/
}

void ModuleEditor::render() {
	auto ModRender = app->getModule<ModuleD3D12>();
    D3D12_CPU_DESCRIPTOR_HANDLE rtv = ModRender->getCurrentRTV();
	imguiPass->record(ModRender->getCommandList(), rtv);
}

void ModuleEditor::postRender() {

}

