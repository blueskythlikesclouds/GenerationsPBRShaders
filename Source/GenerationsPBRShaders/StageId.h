#pragma once

class StageId
{
    static bool changed;
    static std::string stageId;
    
public:
    static bool isEmpty();
    static bool hasChanged();
    static const std::string& get();
    static void update();
};
