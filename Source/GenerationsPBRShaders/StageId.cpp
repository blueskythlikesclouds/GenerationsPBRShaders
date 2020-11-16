#include "StageId.h"

const char* const STAGE_ID = (const char*)0x1E774D4;

bool StageId::changed;
std::string StageId::stageId;

bool StageId::isEmpty()
{
    return stageId.empty();
}

bool StageId::hasChanged()
{
    return changed;
}

const std::string& StageId::get()
{
    return stageId;
}

void StageId::update()
{
    changed = stageId != STAGE_ID;
    if (changed)
        stageId = STAGE_ID;
}
