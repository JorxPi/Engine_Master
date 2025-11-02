#include "Globals.h"
#include "LogConsole.h"

LogConsole::LogConsole() :
    autoScroll(true)
{
}

void LogConsole::addLog(const std::string& message)
{
    logs.push_back(message);
}

void LogConsole::clear()
{
    logs.clear();
}

void LogConsole::draw(const char* title, bool* pOpen)
{
    ImGui::SetNextWindowSize(ImVec2(600, 200), ImGuiCond_FirstUseEver);

    if (!ImGui::Begin(title, pOpen))
    {
        ImGui::End();
        return;
    }

    if (ImGui::Button("Clear")) clear();
    ImGui::SameLine();
    ImGui::Checkbox("Auto-scroll", &autoScroll);
    ImGui::Separator();

    ImGui::BeginChild("scrolling", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);
    for (const auto& log : logs)
        ImGui::TextUnformatted(log.c_str());

    if (autoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
        ImGui::SetScrollHereY(1.0f);
    ImGui::EndChild();

    ImGui::End();
}

