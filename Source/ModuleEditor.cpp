#include "Globals.h"
#include "Application.h"
#include "ModuleEditor.h"
#include "ModuleD3D12.h"
#include "ModulePipeline.h"
#include "ModuleCameraEditor.h"
#include "imgui.h"
#include "ImGuizmo.h"
#include <thread>


ModuleEditor::ModuleEditor(HWND windowHandle)
    : hWnd(windowHandle) {
}

bool ModuleEditor::postInit() {
	auto modRender = app->getModule<ModuleD3D12>();
    if (!modRender) return false;

    imguiPass = std::make_unique<ImGuiPass>(modRender->getDevice(), modRender->getWindowHandle());
    if (imguiPass) LOG("Console initialized successfully!");

    return imguiPass != nullptr;
}

void ModuleEditor::preRender() {
    if (!imguiPass) return;
    imguiPass->startFrame();
    ImGuizmo::BeginFrame();

    RECT rc{};
    GetClientRect(hWnd, &rc);
    float w = float(rc.right - rc.left);
    float h = float(rc.bottom - rc.top);

    ImGuizmo::SetRect(0, 0, w, h);

    auto* pipe = app->getModule<ModulePipeline>();
    auto* cam = app->getModule<ModuleCameraEditor>();

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
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Camera")) 
        {
            ImGui::MenuItem("Camera Controls", nullptr, &showCameraWindow);
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Texture"))
        {
            ImGui::MenuItem("Samples", nullptr, &showTextureSamples);
            ImGui::MenuItem("Debug Grid", nullptr, &showTextureGrid);
            ImGui::MenuItem("Change Texture", nullptr, &showTextureChange);
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Model"))
        {
            ImGui::MenuItem("Geometry Viewer", nullptr, &showGeometryViewer);
            ImGui::EndMenu();
        }

        ImGui::EndMainMenuBar();

        drawDocSpace();

    }

    drawConsoleWindow();
    drawImGuiDocWindow();
    drawAboutWindow();

    drawConfigWindow();

    drawCameraWindow(cam);

    drawTextureSamplesWindow(pipe);
    drawTextureGridWindow(pipe);
    //drawTextureChangeWindow(pipe);

    drawGeometryViewerWindow(pipe, cam);
}

void ModuleEditor::render() {
	auto modRender = app->getModule<ModuleD3D12>();
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
    ImGuiDockNodeFlags dockspaceFlags =
        ImGuiDockNodeFlags_PassthruCentralNode |
        ImGuiDockNodeFlags_NoDockingOverCentralNode;

    ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport(), dockspaceFlags);
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

        if (ImGui::CollapsingHeader("Application"))
            drawAppInfo();

        if (ImGui::CollapsingHeader("Window"))
            drawWindowOptions();

        if (ImGui::CollapsingHeader("Hardware"))
            drawHardwareOptions();

        ImGui::End();
    }
}

void ModuleEditor::drawAppInfo() const {
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
    ImGui::PlotHistogram( "##Framerate", fps_log.data(), static_cast<int>(fps_log.size()), logIndex, nullptr, 0.0f, 100.0f, graphSize);
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

void ModuleEditor::drawTextureSamplesWindow(ModulePipeline* pipe) {
    if (!showTextureSamples) return;

    if (!ImGui::Begin("Samples", &showTextureSamples)) { ImGui::End(); return; }

    if (pipe)
    {
        static int samplerIdx = 0;
        const char* items[] = {
            "Linear Wrap",
            "Linear Clamp",
            "Point Wrap",
            "Point Clamp"
        };

        if (ImGui::Combo("Sampler Mode", &samplerIdx, items, IM_ARRAYSIZE(items)))
        {
            pipe->setSamplerIndex(samplerIdx);
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

/*void ModuleEditor::drawTextureChangeWindow(ModulePipeline* pipe)
{
    if (!showTextureChange) return;

    if (!ImGui::Begin("Change Texture", &showTextureChange)) { ImGui::End(); return; }

    if (pipe) {

        static int texIdx = 0;
        const char* items[] = { "Popcorn (JPG)", "Dog (DDS)" };

        if (ImGui::Combo("Texture", &texIdx, items, IM_ARRAYSIZE(items)))
        {
            const wchar_t* path = (texIdx == 0) ? L"Assets/Textures/popcorn.jpg" : L"Assets/Textures/dog.dds";

            if (!pipe->setTextureFromFile(path))
            {
                LOG("Failed to load texture: %ls", path);
            }
        }

        ImGui::Text("Current: %ls", pipe->getTexturePath());
        ImGui::End();
    }
}*/

void ModuleEditor::drawGeometryViewerWindow(ModulePipeline* pipe, ModuleCameraEditor* cam)
{
    if (!showGeometryViewer || !pipe || !cam) return;

    if (!ImGui::Begin("Geometry Viewer", &showGeometryViewer))
    {
        ImGui::End();
        return;
    }

    Model& model = pipe->getModel();

    static ImGuizmo::OPERATION op = ImGuizmo::TRANSLATE;
    static ImGuizmo::MODE mode = ImGuizmo::LOCAL;

    const bool rmbDown = (GetAsyncKeyState(VK_RBUTTON) & 0x8000) != 0;
    if (!rmbDown) {
        if (ImGui::IsKeyPressed(ImGuiKey_W)) op = ImGuizmo::TRANSLATE;
        if (ImGui::IsKeyPressed(ImGuiKey_E)) op = ImGuizmo::ROTATE;
        if (ImGui::IsKeyPressed(ImGuiKey_R)) op = ImGuizmo::SCALE;
    }

    ImGui::RadioButton("Translate", (int*)&op, (int)ImGuizmo::TRANSLATE); ImGui::SameLine();
    ImGui::RadioButton("Rotate", (int*)&op, (int)ImGuizmo::ROTATE);    ImGui::SameLine();
    ImGui::RadioButton("Scale", (int*)&op, (int)ImGuizmo::SCALE);

    ImGui::RadioButton("Local", (int*)&mode, (int)ImGuizmo::LOCAL); ImGui::SameLine();
    ImGui::RadioButton("World", (int*)&mode, (int)ImGuizmo::WORLD);

    DirectX::SimpleMath::Matrix objectMatrix = model.getModelMatrix();

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

    ImGuizmo::SetDrawlist(ImGui::GetForegroundDrawList());
    ImGuizmo::SetOrthographic(false);

    const auto& view = cam->getViewMatrix();
    const auto& proj = cam->getProjectionMatrix();

    ImGuizmo::Manipulate((const float*)&view, (const float*)&proj, op, mode, (float*)&objectMatrix);

    if (ImGuizmo::IsUsing())
        model.setModelMatrix(objectMatrix);

    ImGui::End();
}