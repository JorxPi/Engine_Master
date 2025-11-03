#include "Globals.h"
#include "Application.h"
#include "ModuleEditor.h"
#include "ModuleD3D12.h"
#include "imgui.h"
#include <thread>


ModuleEditor::ModuleEditor(HWND windowHandle)
    : hWnd(windowHandle) {
}

bool ModuleEditor::postInit() {
	auto modRender = app->getModule<ModuleD3D12>();
	if (modRender) {imguiPass = std::make_unique<ImGuiPass>(modRender->getDevice(), modRender->getWindowHandle());}

	LOG("Console initialized successfully!");

	return true;
}

void ModuleEditor::preRender() {
    imguiPass->startFrame();

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

        ImGui::EndMainMenuBar();

        ImGuiDockNodeFlags dockspaceFlags =
            ImGuiDockNodeFlags_PassthruCentralNode |
            ImGuiDockNodeFlags_NoDockingOverCentralNode;

        ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport(), dockspaceFlags);

    }

    if (showConsole)
        logConsole.draw("Console", &showConsole);

    if (showDemo)
        ImGui::ShowDemoWindow(&showDemo);

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

void ModuleEditor::render() {
	auto modRender = app->getModule<ModuleD3D12>();
    D3D12_CPU_DESCRIPTOR_HANDLE rtv = modRender->getCurrentRTV();
	imguiPass->record(modRender->getCommandList(), rtv);
}

void ModuleEditor::postRender() {

}

void ModuleEditor::logg(const char* format, ...)
{
    char buffer[1024];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    logConsole.addLog(buffer);
}

void ModuleEditor::drawAppInfo() const
{
    static std::array<float, 100> fps_log = {};
    static std::array<float, 100> ms_log = {};
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
    ms_log[logIndex] = ms;
    logIndex = (logIndex + 1) % static_cast<int>(fps_log.size());

    const float graphWidth = ImGui::GetContentRegionAvail().x;
    ImVec2 graphSize(graphWidth, 100);

    ImGui::Text("Framerate: %.1f FPS", fps);
    ImGui::PlotHistogram( "##Framerate", fps_log.data(), static_cast<int>(fps_log.size()), logIndex, nullptr, 0.0f, 100.0f, graphSize);

    ImGui::Text("Milliseconds: %.1f ms", ms);
    graphSize.y = 80;
    ImGui::PlotHistogram( "##Milliseconds", ms_log.data(), static_cast<int>(ms_log.size()), logIndex, nullptr, 0.0f, 40.0f, graphSize);
}

void ModuleEditor::drawWindowOptions()
{
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
            // store current windowed position before going fullscreen
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

            // restore to previous windowed position and size
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

        // update last windowed rect so it's remembered correctly
        GetWindowRect(hWnd, &lastWindowedRect);
    }
}



void ModuleEditor::drawHardwareOptions() const
{
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