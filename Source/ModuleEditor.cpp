#include "Globals.h"
#include "Application.h"
#include "ModuleEditor.h"
#include "ModuleD3D12.h"
#include "ModulePipeline.h"
#include "ModuleCameraEditor.h"
#include "imgui.h"
#include "ImGuizmo.h"
#include <thread>
#include "ModuleShaderDescriptors.h"
#include "ModuleSampler.h"
#include <imgui_internal.h>

ModuleEditor::ModuleEditor(HWND windowHandle)
    : hWnd(windowHandle) {
}

bool ModuleEditor::postInit() {
    auto modRender = app->getModule<ModuleD3D12>();
    auto shaderDesc = app->getModule<ModuleShaderDescriptors>();

    uint32_t imguiIdx = shaderDesc->allocIndex();

    imguiPass = std::make_unique<ImGuiPass>(modRender->getDevice(), modRender->getWindowHandle(), shaderDesc->getCpuHandle(imguiIdx), shaderDesc->getGpuHandle(imguiIdx));

    return imguiPass != nullptr;
}

void ModuleEditor::preRender() {
    if (!imguiPass) return;
    imguiPass->startFrame();
    ImGuizmo::BeginFrame();

    drawDocSpace();

    RECT rc{};
    GetClientRect(hWnd, &rc);
    float w = float(rc.right - rc.left);
    float h = float(rc.bottom - rc.top);

    auto* pipe = app->getModule<ModulePipeline>();
    auto* cam = app->getModule<ModuleCameraEditor>();

    focusOnModel(pipe, cam);

    if (ImGui::BeginMainMenuBar())
    {
        if (ImGui::BeginMenu("View"))
        {
            ImGui::MenuItem("Console", nullptr, &showConsole);
            ImGui::MenuItem("Documentation (ImGui Demo)", nullptr, &showDemo);
            ImGui::MenuItem("About", nullptr, &showAbout); 

            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Options"))
        {
            ImGui::MenuItem("Configuration", nullptr, &showConfig);
            ImGui::MenuItem("Application", nullptr, &showApplication);
            ImGui::MenuItem("Debug Grid", nullptr, &showTextureGrid);

            if (ImGui::MenuItem("Reset Layout"))
                requestResetLayout = true;

            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Camera")) 
        {
            ImGui::MenuItem("Camera Controls", nullptr, &showCameraWindow);
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Model"))
        {
            ImGui::MenuItem("Geometry Viewer", nullptr, &showGeometryViewer);
            ImGui::MenuItem("Phong Controls", nullptr, &showPhongControls);
            ImGui::MenuItem("Lights", nullptr, &showLightsWindow);
            ImGui::EndMenu();
        }


        ImGui::EndMainMenuBar();

        drawSceneWindow(pipe, cam);

    }

    drawConsoleWindow();
    drawImGuiDocWindow();
    drawAboutWindow();

    drawConfigWindow();
    drawAppInfo();

    drawCameraWindow(cam);

    drawTextureGridWindow(pipe);

    drawGeometryViewerWindow(pipe, cam);
    drawPhongControlsWindow(pipe, cam);
    drawLightsWindow(pipe);

    //ImGizmo
    drawGizmo(pipe, cam);
    updateGizmoSelection(pipe);
    updateGizmoHotkeys();
}

void ModuleEditor::render() {
	auto modRender = app->getModule<ModuleD3D12>();
    auto* shaderDesc = app->getModule<ModuleShaderDescriptors>();
    auto* samplerMod = app->getModule<ModuleSampler>();

    ID3D12GraphicsCommandList* cmd = modRender->getCommandList();

    RECT rc{};
    GetClientRect(modRender->getWindowHandle(), &rc);
    float w = float(rc.right - rc.left);
    float h = float(rc.bottom - rc.top);

    D3D12_VIEWPORT vp{ 0.0f, 0.0f, w, h, 0.0f, 1.0f };
    D3D12_RECT sc{ 0, 0, (LONG)w, (LONG)h };
    cmd->RSSetViewports(1, &vp);
    cmd->RSSetScissorRects(1, &sc);

    ID3D12DescriptorHeap* heaps[2] = {};
    uint32_t heapCount = 0;

    if (shaderDesc && shaderDesc->getHeap())
        heaps[heapCount++] = shaderDesc->getHeap();

    if (samplerMod && samplerMod->getHeap())
        heaps[heapCount++] = samplerMod->getHeap();

    if (heapCount > 0)
        cmd->SetDescriptorHeaps(heapCount, heaps);

    D3D12_CPU_DESCRIPTOR_HANDLE rtv = modRender->getCurrentRTV();
	imguiPass->record(modRender->getCommandList(), rtv);
}

void ModuleEditor::logg(const char* format, ...) {
    char buffer[1024];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    logConsole.addLog(buffer);
}

void ModuleEditor::drawDocSpace() {
    ImGuiViewport* vp = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(vp->WorkPos);
    ImGui::SetNextWindowSize(vp->WorkSize);
    ImGui::SetNextWindowViewport(vp->ID);

    ImGuiWindowFlags hostFlags =
        ImGuiWindowFlags_NoDocking |
        ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoBringToFrontOnFocus |
        ImGuiWindowFlags_NoNavFocus;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));

    ImGui::Begin("DockSpaceHost", nullptr, hostFlags);

    ImGui::PopStyleVar(3);

    ImGuiID dockspaceId = ImGui::GetID("MainDockSpace");

    if (requestResetLayout)
    {
        ImGui::ClearIniSettings();

        ImGui::DockBuilderRemoveNode(dockspaceId);

        dockBuilt = false;
        requestResetLayout = false;
    }

    ImGuiDockNodeFlags dockFlags = ImGuiDockNodeFlags_PassthruCentralNode;
    ImGui::DockSpace(dockspaceId, ImVec2(0, 0), dockFlags);

    if (!dockBuilt)
    {
        buildDefaultLayout(dockspaceId);
        dockBuilt = true;
    }

    ImGui::End();
}

void ModuleEditor::buildDefaultLayout(ImGuiID dockspaceId)
{
    ImGui::DockBuilderRemoveNode(dockspaceId);
    ImGui::DockBuilderAddNode(dockspaceId, ImGuiDockNodeFlags_DockSpace);
    ImGui::DockBuilderSetNodeSize(dockspaceId, ImGui::GetMainViewport()->WorkSize);

    ImGuiID dockMain = dockspaceId;
    ImGuiID dockRight = ImGui::DockBuilderSplitNode(dockMain, ImGuiDir_Right, 0.28f, nullptr, &dockMain);
    ImGuiID dockBottom = ImGui::DockBuilderSplitNode(dockMain, ImGuiDir_Down, 0.30f, nullptr, &dockMain);

    ImGuiID dockBottomLeft = ImGui::DockBuilderSplitNode(dockBottom, ImGuiDir_Left, 0.50f, nullptr, &dockBottom);
    ImGuiID dockBottomRight = dockBottom;

    ImGui::DockBuilderDockWindow("Scene", dockMain);

    ImGui::DockBuilderDockWindow("Console", dockBottomLeft);
    ImGui::DockBuilderDockWindow("Application", dockBottomRight);

    ImGui::DockBuilderDockWindow("Geometry Viewer", dockRight);
    ImGui::DockBuilderDockWindow("Phong Controls", dockRight);

    ImGui::DockBuilderFinish(dockspaceId);
}



void ModuleEditor::drawConsoleWindow() {
    if (showConsole)
        logConsole.draw("Console", &showConsole);
}

void ModuleEditor::drawImGuiDocWindow() {
    if (showDemo)
        ImGui::ShowDemoWindow(&showDemo);
}

void ModuleEditor::drawAboutWindow() {
    if (showAbout)
    {
        ImGui::SetNextWindowSize(ImVec2(400, 250), ImGuiCond_FirstUseEver);
        ImGui::Begin("About", &showAbout);

        ImGui::Text("My Engine: %s", "Engine_Master");

        ImGui::Separator();
        ImGui::TextWrapped("This is the Engine in C++ using DirectX12 for rendering that I will be updating and using to learn during my Master in Advanced Programming for AAA Videogames in UPC");

        ImGui::Separator();
        ImGui::Text("Author: %s", "Jordi Pi Moreno");

        ImGui::Separator();
        ImGui::Text("Libraries used:");
        ImGui::BulletText("Dear ImGui %s", IMGUI_VERSION);
        ImGui::BulletText("DirectX 12 (D3D12)");
        ImGui::BulletText("C++ Standard Library");

        ImGui::Separator();
        ImGui::Text("License: MIT License");
        ImGui::TextDisabled("(c) 2025 Jordi Pi. All rights reserved.");

        ImGui::End();
    }
}

void ModuleEditor::drawConfigWindow() {
    if (showConfig)
    {
        ImGui::Begin("Configuration", &showConfig);

        if (ImGui::CollapsingHeader("Window"))
            drawWindowOptions();

        if (ImGui::CollapsingHeader("Hardware"))
            drawHardwareOptions();

        ImGui::End();
    }
}

void ModuleEditor::drawAppInfo() {
    if (!showApplication) return;

    if (!ImGui::Begin("Application", &showApplication)) { ImGui::End(); return; }

    static std::array<float, 100> fps_log = {};
    static int logIndex = 0;
    static int maxFPS = 60;

    const float fps = ImGui::GetIO().Framerate;
    const float ms = 1000.0f / fps;

    ImGui::Text("Limit Framerate");
    ImGui::SliderInt("##FPSLimit", &maxFPS, 30, 240, "%d FPS");

    if (maxFPS > 0)
    {
        const float targetFrameTime = 1000.0f / maxFPS;
        const float currentFrameTime = ms;

        if (currentFrameTime < targetFrameTime)
        {
            const DWORD sleepTime = static_cast<DWORD>(targetFrameTime - currentFrameTime);
            Sleep(sleepTime);
        }
    }

    ImGui::Separator();

    fps_log[logIndex] = fps;
    logIndex = (logIndex + 1) % static_cast<int>(fps_log.size());

    const float graphWidth = ImGui::GetContentRegionAvail().x;
    ImVec2 graphSize(graphWidth, 100);

    ImGui::Text("Framerate: %.1f FPS", fps);
    ImGui::PlotHistogram("##Framerate", fps_log.data(), static_cast<int>(fps_log.size()), logIndex, nullptr, 0.0f, 100.0f, graphSize);

    ImGui::End();
    
}

void ModuleEditor::drawWindowOptions() {
    static bool fullscreen = false;
    static bool resizable = true;
    static int width = 1280;
    static int height = 720;

    static RECT lastWindowedRect = { 100, 100, 100 + width, 100 + height };

    if (!fullscreen && resizable)
    {
        WINDOWPLACEMENT placement = { sizeof(WINDOWPLACEMENT) };
        if (GetWindowPlacement(hWnd, &placement))
        {
            if (placement.showCmd != SW_MAXIMIZE)
            {
                RECT rect;
                if (GetWindowRect(hWnd, &rect))
                {
                    width = rect.right - rect.left;
                    height = rect.bottom - rect.top;
                }
            }
        }
    }

    if (ImGui::Checkbox("Fullscreen", &fullscreen))
    {
        if (fullscreen)
        {
            GetWindowRect(hWnd, &lastWindowedRect);
            resizable = false;
            LOG("Switching to fullscreen mode");

            HMONITOR monitor = MonitorFromWindow(hWnd, MONITOR_DEFAULTTOPRIMARY);
            MONITORINFO mi = { sizeof(mi) };
            if (GetMonitorInfo(monitor, &mi))
            {
                SetWindowLong(hWnd, GWL_STYLE, WS_POPUP);
                SetWindowPos(hWnd, HWND_TOP,
                    mi.rcMonitor.left, mi.rcMonitor.top,
                    mi.rcMonitor.right - mi.rcMonitor.left,
                    mi.rcMonitor.bottom - mi.rcMonitor.top,
                    SWP_FRAMECHANGED | SWP_SHOWWINDOW);
            }
        }
        else
        {
            LOG("Exiting fullscreen");

            SetWindowLong(hWnd, GWL_STYLE, WS_OVERLAPPEDWINDOW);
            int restoredWidth = lastWindowedRect.right - lastWindowedRect.left;
            int restoredHeight = lastWindowedRect.bottom - lastWindowedRect.top;

            SetWindowPos(hWnd, HWND_TOP,
                lastWindowedRect.left, lastWindowedRect.top,
                restoredWidth, restoredHeight,
                SWP_FRAMECHANGED | SWP_SHOWWINDOW);
        }
    }

    ImGui::SameLine();

    if (ImGui::Checkbox("Resizable", &resizable))
    {
        if (resizable)
        {
            fullscreen = false;
            LOG("Enabling resizable window");
            SetWindowLong(hWnd, GWL_STYLE, WS_OVERLAPPEDWINDOW);

            RECT r = lastWindowedRect; 
            int w = r.right - r.left;
            int h = r.bottom - r.top;
            SetWindowPos(hWnd, nullptr, r.left, r.top, w, h, SWP_FRAMECHANGED | SWP_SHOWWINDOW);
        }
        else
        {
            LOG("Disabling resizable window");
            LONG style = GetWindowLong(hWnd, GWL_STYLE);
            style &= ~WS_THICKFRAME; 
            SetWindowLong(hWnd, GWL_STYLE, style);

            RECT r; GetWindowRect(hWnd, &r);
            SetWindowPos(hWnd, nullptr, r.left, r.top, r.right - r.left, r.bottom - r.top,
                SWP_FRAMECHANGED | SWP_SHOWWINDOW);
        }
    }

    ImGui::Text("Window Size:");
    bool sizeChanged = false;
    sizeChanged |= ImGui::SliderInt("Width", &width, 640, 2560);
    sizeChanged |= ImGui::SliderInt("Height", &height, 480, 1440);

    if (resizable && sizeChanged)
    {
        LOG("Resizing window to %dx%d", width, height);
        RECT rect;
        GetWindowRect(hWnd, &rect);
        SetWindowPos(hWnd, nullptr, rect.left, rect.top, width, height, SWP_SHOWWINDOW);

        GetWindowRect(hWnd, &lastWindowedRect);
    }
}



void ModuleEditor::drawHardwareOptions() const {
    ImGui::Text("CPU cores: %d", std::thread::hardware_concurrency());

    MEMORYSTATUSEX memInfo;
    memInfo.dwLength = sizeof(MEMORYSTATUSEX);
    GlobalMemoryStatusEx(&memInfo);
    ImGui::Text("System RAM: %.1f GB", static_cast<double>(memInfo.ullTotalPhys) / (1024.0f * 1024.0f * 1024.0f));

    ImGui::Separator();

    ComPtr<IDXGIFactory> dxgiFactory;
    if (SUCCEEDED(CreateDXGIFactory(IID_PPV_ARGS(&dxgiFactory))))
    {
        ComPtr<IDXGIAdapter> adapter;
        if (SUCCEEDED(dxgiFactory->EnumAdapters(0, &adapter)))
        {
            DXGI_ADAPTER_DESC desc;
            adapter->GetDesc(&desc);
            ImGui::Text("GPU: %ls", desc.Description);
            ImGui::Text("VRAM: %.1f MB", static_cast<double>(desc.DedicatedVideoMemory) / (1024.0f * 1024.0f));
        }
    }
}

void ModuleEditor::drawCameraWindow(ModuleCameraEditor* camMod) {
    if (!showCameraWindow) return;

    if (!ImGui::Begin("Camera Controls", &showCameraWindow)) { ImGui::End(); return; }

    if (camMod)
    {
        const Camera& cam = camMod->readCamera();

        // --- Position ---
        {
            auto pos = cam.position;
            if (ImGui::DragFloat3("Position", &pos.x, 0.1f))
                camMod->setPosition(pos);
        }

        ImGui::Separator();

        // --- LookAt ---
        {
            ImGui::Text("Look At");

            static float targetDistance = 10.0f;
            static Vector3 target = Vector3::Zero;
            static bool wasEditing = false;

            Vector3 forward = Vector3::Transform(Vector3::Forward, cam.orientation);
            forward.Normalize();

            if (!wasEditing) {
                Vector3 derivedTarget = cam.position + forward * targetDistance;
                target = derivedTarget;
            }

            ImGui::DragFloat3("Target", &target.x, 0.1f);

            const bool editingNow = ImGui::IsItemActive();
            const bool finishedEdit = ImGui::IsItemDeactivatedAfterEdit();
            wasEditing = editingNow;

            // when user finishes editing, rotate camera to look at it
            if (finishedEdit)
            {
                targetDistance = (target - cam.position).Length();
                if (targetDistance < 0.001f) targetDistance = 0.001f;

                camMod->setLookAt(target, Vector3::Up);
            }
        }

        ImGui::Separator();

        // --- FOV and Planes ---
        {
            float fovX = cam.fovX;
            if (ImGui::SliderAngle("FOV X (horizontal)", &fovX, 10.0f, 120.0f))
                camMod->setFOV(fovX);

            float nearP = cam.nearPlane;
            float farP = cam.farPlane;

            bool planesChanged = false;
            planesChanged |= ImGui::DragFloat("Near Plane", &nearP, 0.01f, 0.001f, 100.0f);
            planesChanged |= ImGui::DragFloat("Far Plane", &farP, 1.0f, 1.0f, 5000.0f);

            if (planesChanged)
                camMod->setPlaneDistances(nearP, farP);
        }
    }

    ImGui::End();
}

void ModuleEditor::drawTextureGridWindow(ModulePipeline* pipe) {
    if (!showTextureGrid) return;
    
    if (!ImGui::Begin("Grid", &showTextureGrid)) { ImGui::End(); return; }

    if (pipe)
    {
        bool grid = pipe->getShowGrid();
        if (ImGui::Checkbox("Show Grid", &grid))
            pipe->setShowGrid(grid);

        bool axis = pipe->getShowAxis();
        if (ImGui::Checkbox("Show Axis", &axis))
            pipe->setShowAxis(axis);
    }

    ImGui::End();
    
}

void ModuleEditor::drawGeometryViewerWindow(ModulePipeline* pipe, ModuleCameraEditor* cam)
{
    if (!showGeometryViewer) return;

    if (!ImGui::Begin("Geometry Viewer", &showGeometryViewer))
    {
        ImGui::End();
        return;
    }

    if (!pipe || !cam)
    {
        ImGui::End();
        return;
    }

    Model& model = pipe->getModel();

    ImGui::RadioButton("Translate", (int*)&gizmoOp, (int)ImGuizmo::TRANSLATE); ImGui::SameLine();
    ImGui::RadioButton("Rotate", (int*)&gizmoOp, (int)ImGuizmo::ROTATE);    ImGui::SameLine();
    ImGui::RadioButton("Scale", (int*)&gizmoOp, (int)ImGuizmo::SCALE);

    ImGui::RadioButton("Local", (int*)&gizmoMode, (int)ImGuizmo::LOCAL); ImGui::SameLine();
    ImGui::RadioButton("World", (int*)&gizmoMode, (int)ImGuizmo::WORLD);

    ImGui::Checkbox("Show Gizmo", &showGizmo);

    Matrix objectMatrix = model.getModelMatrix();
    float tr[3], rt[3], sc[3];
    ImGuizmo::DecomposeMatrixToComponents((float*)&objectMatrix, tr, rt, sc);

    bool changed = false;
    changed |= ImGui::DragFloat3("Tr", tr, 0.01f);
    changed |= ImGui::DragFloat3("Rt", rt, 0.1f);
    changed |= ImGui::DragFloat3("Sc", sc, 0.01f);

    if (changed)
    {
        ImGuizmo::RecomposeMatrixFromComponents(tr, rt, sc, (float*)&objectMatrix);
        model.setModelMatrix(objectMatrix);
    }

    ImGui::End();
}

void ModuleEditor::drawPhongControlsWindow(ModulePipeline* pipe, ModuleCameraEditor* cam)
{
    if (!showPhongControls) return;

    if (!ImGui::Begin("Phong Controls", &showPhongControls))
    {
        ImGui::End();
        return;
    }

    if (!pipe || !cam)
    {
        ImGui::End();
        return;
    }

    auto& phong = pipe->editPhong();

    // Light
    if (ImGui::CollapsingHeader("Light", ImGuiTreeNodeFlags_DefaultOpen))
    {
        bool changedDir = ImGui::DragFloat3("Direction", (float*)&phong.lightDir, 0.01f, -1.0f, 1.0f);
        ImGui::SameLine();
        if (ImGui::SmallButton("Normalize"))
        {
            phong.lightDir.Normalize();
            changedDir = true;
        }
        if (changedDir)
        {
            phong.lightDir.Normalize();
        }

        ImGui::ColorEdit3("Light Color", (float*)&phong.lightColor);
        ImGui::DragFloat("Light Intensity", &phong.lightIntensity, 0.1f, 0.0f, 20.0f);

        ImGui::ColorEdit3("Ambient", (float*)&phong.ambient);
        ImGui::DragFloat("Ambient Intensity", &phong.ambientIntensity, 0.05f, 0.0f, 5.0f);
    }

    ImGui::Separator();

    // Sampler
    if (ImGui::CollapsingHeader("Sampling", ImGuiTreeNodeFlags_DefaultOpen))
    {
        static const char* samplerItems[] = {
            "Point Wrap",
            "Point Clamp",
            "Linear Wrap",
            "Linear Clamp"
        };

        int s = phong.samplerIndex;
        if (ImGui::Combo("Sampler", &s, samplerItems, IM_ARRAYSIZE(samplerItems)))
        {
            pipe->setSamplerIndex(s);
        }
    }

    ImGui::Separator();

    // Material override
    if (ImGui::CollapsingHeader("Material Override", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::Checkbox("Show Texture Material", &phong.useOverride);

        if (phong.useOverride)
        {
            float col[3] = {
                phong.overrideMat.diffuseColour.x,
                phong.overrideMat.diffuseColour.y,
                phong.overrideMat.diffuseColour.z,
            };

            if (ImGui::ColorEdit3("Diffuse Colour", col, ImGuiColorEditFlags_NoInputs))
            {
                phong.overrideMat.diffuseColour = DirectX::XMFLOAT3(col[0], col[1], col[2]);
            }

            bool hasTex = (phong.overrideMat.hasDiffuseTex != 0);
            if (ImGui::Checkbox("Use Diffuse Texture", &hasTex))
                phong.overrideMat.hasDiffuseTex = hasTex ? TRUE : FALSE;

            ImGui::Separator();

            float spec[3] = {
                phong.overrideMat.specularColour.x,
                phong.overrideMat.specularColour.y,
                phong.overrideMat.specularColour.z
            };

            if (ImGui::ColorEdit3("Specular Colour (F0)", spec, ImGuiColorEditFlags_NoInputs))
                phong.overrideMat.specularColour = DirectX::XMFLOAT3(spec[0], spec[1], spec[2]);

            ImGui::DragFloat("Shininess", &phong.overrideMat.shininess, 1.0f, 1.0f, 512.0f);
        }
    }

    ImGui::End();
}

void ModuleEditor::drawLightsWindow(ModulePipeline* pipe)
{
    if (!showLightsWindow) return;
    if (!ImGui::Begin("Lights", &showLightsWindow)) { ImGui::End(); return; }
    if (!pipe) { ImGui::TextDisabled("No pipeline"); ImGui::End(); return; }

    LightSystem& lightSystem = pipe->editLightSystem();

    // ----- Ambient -----
    {
        Vector3 ambientColor = lightSystem.getAmbientColor();
        float ambientIntensity = lightSystem.getAmbientIntensity();

        float col[3] = { ambientColor.x, ambientColor.y, ambientColor.z };
        if (ImGui::ColorEdit3("Ambient Color", col))
            ambientColor = Vector3(col[0], col[1], col[2]);

        if (ImGui::DragFloat("Ambient Intensity", &ambientIntensity, 0.05f, 0.0f, 10.0f))
        {
        }

        lightSystem.setAmbient(ambientColor, ambientIntensity);
    }

    ImGui::Separator();

    auto editVector3 = [](const char* label, Vector3& v, float speed)
        {
            return ImGui::DragFloat3(label, &v.x, speed);
        };

    // ----- Directional -----
    {
        ImGui::Text("Directional");
        OwnerId ownerId = pipe->getDirectionalOwner();
        LightId lightId = pipe->getDirectionalLight();

        ManualTransform transform{};
        if (lightSystem.getOwnerTransform(ownerId, transform))
        {
            editVector3("Direction (forward)", transform.forward, 0.01f);
            if (ImGui::SmallButton("Normalize Direction"))
                transform.forward.Normalize();

            lightSystem.setOwnerTransform(ownerId, transform);

            if (ImGui::SmallButton("Gizmo##Dir"))
            {
                gizmoTarget = GizmoTarget::DirectionalLight;
                hasSelection = true;
                gizmoOp = ImGuizmo::ROTATE;
            }

            ImGui::Checkbox("Directional Debug##Draw", &pipe->editShowDirectionalLightDebugDraw());
        }

        if (LightInstance* instance = lightSystem.getLight(lightId))
        {
            ImGui::Checkbox("Enabled##Dir", &instance->lightComponent.common.enabled);

            float col[3] = {
                instance->lightComponent.common.color.x,
                instance->lightComponent.common.color.y,
                instance->lightComponent.common.color.z
            };
            if (ImGui::ColorEdit3("Color##Dir", col))
                instance->lightComponent.common.color = Vector3(col[0], col[1], col[2]);

            ImGui::DragFloat("Intensity##Dir", &instance->lightComponent.common.intensity, 0.1f, 0.0f, 200.0f);
        }
    }

    ImGui::Separator();

    // ----- Point -----
    {
        ImGui::Text("Point");
        OwnerId ownerId = pipe->getPointOwner();
        LightId lightId = pipe->getPointLight();

        ManualTransform transform{};
        if (lightSystem.getOwnerTransform(ownerId, transform))
        {
            editVector3("Position##Point", transform.position, 0.05f);
            lightSystem.setOwnerPosition(ownerId, transform.position);

            if (ImGui::SmallButton("Gizmo##Point"))
            {
                gizmoTarget = GizmoTarget::PointLight;
                hasSelection = true;
                gizmoOp = ImGuizmo::TRANSLATE;
            }

            ImGui::Checkbox("Point Debug##Draw", &pipe->editShowPointLightDebugDraw());
        }

        if (LightInstance* instance = lightSystem.getLight(lightId))
        {
            auto& common = instance->lightComponent.common;
            ImGui::Checkbox("Enabled##Point", &common.enabled);

            float col[3] = { common.color.x, common.color.y, common.color.z };
            if (ImGui::ColorEdit3("Color##Point", col))
                common.color = Vector3(col[0], col[1], col[2]);

            ImGui::DragFloat("Intensity##Point", &common.intensity, 0.1f, 0.0f, 500.0f);

            if (instance->lightComponent.type == LightType::Point)
            {
                ImGui::DragFloat("Radius##Point",
                    &instance->lightComponent.parameters.point.radius,
                    0.1f, 0.0f, 200.0f);
            }
            else
            {
                ImGui::TextDisabled("This light is not of type Point.");
            }
        }
    }

    ImGui::Separator();

    // ----- Spot -----
    {
        ImGui::Text("Spot");
        OwnerId ownerId = pipe->getSpotOwner();
        LightId lightId = pipe->getSpotLight();

        ManualTransform transform{};
        if (lightSystem.getOwnerTransform(ownerId, transform))
        {
            editVector3("Position##Spot", transform.position, 0.05f);
            editVector3("Direction##Spot", transform.forward, 0.01f);
            if (ImGui::SmallButton("Normalize Spot Direction"))
                transform.forward.Normalize();

            lightSystem.setOwnerTransform(ownerId, transform);

            if (ImGui::SmallButton("Gizmo##Spot"))
            {
                gizmoTarget = GizmoTarget::SpotLight;
                hasSelection = true;
                gizmoOp = ImGuizmo::ROTATE; 
            }

            ImGui::Checkbox("Spot Debug##Draw", &pipe->editShowSpotLightDebugDraw());

        }

        if (LightInstance* instance = lightSystem.getLight(lightId))
        {
            auto& common = instance->lightComponent.common;
            ImGui::Checkbox("Enabled##Spot", &common.enabled);

            float col[3] = { common.color.x, common.color.y, common.color.z };
            if (ImGui::ColorEdit3("Color##Spot", col))
                common.color = Vector3(col[0], col[1], col[2]);

            ImGui::DragFloat("Intensity##Spot", &common.intensity, 0.1f, 0.0f, 500.0f);

            if (instance->lightComponent.type == LightType::Spot)
            {
                ImGui::DragFloat("Radius##Spot",
                    &instance->lightComponent.parameters.spot.radius,
                    0.1f, 0.0f, 200.0f);

                ImGui::DragFloat("Inner Angle##Spot",
                    &instance->lightComponent.parameters.spot.innerAngleDegrees,
                    0.1f, 0.0f, 179.0f);

                ImGui::DragFloat("Outer Angle##Spot",
                    &instance->lightComponent.parameters.spot.outerAngleDegrees,
                    0.1f, 0.0f, 179.0f);
            }
            else
            {
                ImGui::TextDisabled("This light is not of type Spot.");
            }
        }
    }

    ImGui::End();
}



void ModuleEditor::focusOnModel(ModulePipeline* pipe, ModuleCameraEditor* cam) {
    ImGuiIO& io = ImGui::GetIO();

    if ((sceneHovered || sceneFocused) && ImGui::IsKeyPressed(ImGuiKey_F) && !io.WantTextInput && !ImGuizmo::IsUsing())
    {
        if (pipe && cam)
        {
            Model& model = pipe->getModel();

            Vector3 worldPivot = model.getModelMatrix().Translation();

            cam->focusOnGeometry(worldPivot);
        }
    }
}

void ModuleEditor::updateGizmoHotkeys()
{
    if (!(sceneHovered || sceneFocused))
        return;

    const bool rmbDown = (GetAsyncKeyState(VK_RBUTTON) & 0x8000) != 0;
    if (rmbDown) return;

    ImGuiIO& io = ImGui::GetIO();

    if (io.WantTextInput)
        return;

    if (ImGui::IsAnyItemActive())
        return;

    if (ImGuizmo::IsUsing())
        return;

    if (ImGui::IsKeyPressed(ImGuiKey_W)) { gizmoOp = ImGuizmo::TRANSLATE; hasSelection = true; }
    if (ImGui::IsKeyPressed(ImGuiKey_E)) { gizmoOp = ImGuizmo::ROTATE;    hasSelection = true; }
    if (ImGui::IsKeyPressed(ImGuiKey_R)) { gizmoOp = ImGuizmo::SCALE;     hasSelection = true; }
}

static Vector3 SafeUpFromForward(const Vector3& fwd)
{
    Vector3 f = fwd; f.Normalize();
    return (fabsf(f.Dot(Vector3::Up)) > 0.99f) ? Vector3::Right : Vector3::Up;
}

static Matrix MakeLightMatrix(const Vector3& pos, const Vector3& forward)
{
    Vector3 f = forward; f.Normalize();
    Vector3 upRef = SafeUpFromForward(f);
    return Matrix::CreateWorld(pos, f, upRef);
}

void ModuleEditor::drawGizmo(ModulePipeline* pipe, ModuleCameraEditor* cam)
{
    if (!showGizmo || !pipe || !cam || !hasSelection || !sceneDrawList) return;

    ImGuizmo::SetDrawlist(sceneDrawList);
    ImGuizmo::SetOrthographic(false);
    ImGuizmo::SetRect(sceneCursorScreenPos.x, sceneCursorScreenPos.y, sceneCanvasSize.x, sceneCanvasSize.y);

    const Matrix& view = cam->getViewMatrix();
    const Matrix& proj = cam->getProjectionMatrix();

    // --- MODEL gizmo (your current logic) ---
    if (gizmoTarget == GizmoTarget::Model)
    {
        Model& model = pipe->getModel();
        Matrix objectMatrix = model.getModelMatrix();

        ImGuizmo::Manipulate((const float*)&view, (const float*)&proj, gizmoOp, gizmoMode, (float*)&objectMatrix);

        if (ImGuizmo::IsUsing())
            model.setModelMatrix(objectMatrix);

        return;
    }

    // --- LIGHT gizmo ---
    LightSystem& ls = pipe->editLightSystem();

    OwnerId ownerId = 0;
    switch (gizmoTarget)
    {
    case GizmoTarget::DirectionalLight: ownerId = pipe->getDirectionalOwner(); break;
    case GizmoTarget::PointLight:       ownerId = pipe->getPointOwner();       break;
    case GizmoTarget::SpotLight:        ownerId = pipe->getSpotOwner();        break;
    default: return;
    }

    ManualTransform t{};
    if (!ls.getOwnerTransform(ownerId, t))
        return;

    Matrix m = MakeLightMatrix(t.position, t.forward);

    ImGuizmo::Manipulate((const float*)&view, (const float*)&proj, gizmoOp, gizmoMode, (float*)&m);

    if (ImGuizmo::IsUsing())
    {
        ManualTransform out = t;
        out.position = m.Translation();

        Vector3 newForward = Vector3::TransformNormal(Vector3::Forward, m);
        if (newForward.LengthSquared() > 1e-8f)
            newForward.Normalize();
        else
            newForward = Vector3::Forward;

        out.forward = newForward;

        ls.setOwnerTransform(ownerId, out);
    }
}

void ModuleEditor::updateGizmoSelection(ModulePipeline* pipe)
{
    if (!pipe) return;

    // Esc and left click (without alt pressed) deactivates selection
    if (ImGui::IsKeyPressed(ImGuiKey_Escape))
        hasSelection = false;

    if (!sceneHovered)
        return;

    ImGuiIO& io = ImGui::GetIO();

    if (!ImGui::IsMouseClicked(ImGuiMouseButton_Left))
        return;

    if ((GetAsyncKeyState(VK_MENU) & 0x8000) != 0)
        return;

    if (io.WantCaptureMouse)
        return;

    if (ImGuizmo::IsOver() || ImGuizmo::IsUsing())
        return;

    hasSelection = false;
}

void ModuleEditor::drawSceneWindow(ModulePipeline* pipe, ModuleCameraEditor* cam)
{
    sceneFocused = false;
    sceneHovered = false;
    sceneDrawList = nullptr;

    if (!ImGui::Begin("Scene"))
    {
        ImGui::End();
        return;
    }

    ImVec2 avail = ImGui::GetContentRegionAvail();
    sceneCanvasSize = ImVec2((avail.x > 0) ? avail.x : 0, (avail.y > 0) ? avail.y : 0);
    sceneCursorScreenPos = ImGui::GetCursorScreenPos();

    ImGuiID id = ImGui::GetID("SceneViewport");
    ImGui::BeginChildFrame(id, sceneCanvasSize,
        ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

    sceneFocused = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);
    sceneHovered = ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem);
    sceneDrawList = ImGui::GetWindowDrawList();

    if (pipe && sceneCanvasSize.x > 0.0f && sceneCanvasSize.y > 0.0f)
        pipe->setSceneSize((int)sceneCanvasSize.x, (int)sceneCanvasSize.y);

    bool imageHovered = false;

    if (pipe && pipe->getSceneRT() && pipe->getSceneRT()->isValid())
    {
        auto srv = pipe->getSceneRT()->getSrvGpu();
        ImGui::Image((ImTextureID)srv.ptr, sceneCanvasSize);

        imageHovered = ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem);
    }
    else
    {
        ImGui::TextDisabled("Scene not ready...");
    }

    if (cam)
        cam->setSceneInput(imageHovered, sceneFocused);

    ImGui::EndChildFrame();
    ImGui::End();
}
