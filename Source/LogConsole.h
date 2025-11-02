#pragma once
#include <vector>
#include <string>
#include "imgui.h"

class LogConsole
{
public:
    LogConsole();

    void addLog(const std::string& message);
    void clear();
    void draw(const char* title = "Console", bool* pOpen = nullptr);

private:
    std::vector<std::string> logs;
    bool autoScroll;
};
