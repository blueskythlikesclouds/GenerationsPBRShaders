#include "ArchiveTreePatcher.h"

const std::vector<std::pair<const char*, const std::vector<const char*>>> NODES =
{
    { "PBRSystemCommon", { "SystemCommon" } },
    { "PBRCmn100", { "cmn100" }},
    { "PBRCmn200", { "cmn200" }},
    { "PBRSonic", {"Sonic"}},
    { "PBRSonicClassic", {"SonicClassic"}},
    { "PBREnemyBeeton", {"EnemyBeeton"}},
    { "PBREnemyEFighter", {"EnemyEFighter"}},
};

HOOK(bool, __stdcall, ParseArchiveTree, 0xD4C8E0, void* A1, char* pData, const size_t size, void* pDatabase)
{
    std::string str;
    {
        std::stringstream stream;

        for (auto& node : NODES)
        {
            stream << "  <Node>\n";
            stream << "    <Name>" << node.first << "</Name>\n";
            stream << "    <Archive>" << node.first << "</Archive>\n";
            stream << "    <Order>" << 0 << "</Order>\n";
            stream << "    <DefAppend>" << node.first << "</DefAppend>\n";

            for (auto& archive : node.second)
            {
                stream << "    <Node>\n";
                stream << "      <Name>" << archive << "</Name>\n";
                stream << "      <Archive>" << archive << "</Archive>\n";
                stream << "      <Order>" << 0 << "</Order>\n";
                stream << "    </Node>\n";
            }

            stream << "  </Node>\n";
        }

        str = stream.str();
    }

    const size_t newSize = size + str.size();
    const std::unique_ptr<char[]> pBuffer = std::make_unique<char[]>(newSize);
    memcpy(pBuffer.get(), pData, size);

    char* pInsertionPos = strstr(pBuffer.get(), "<Include>");

    memmove(pInsertionPos + str.size(), pInsertionPos, size - (size_t)(pInsertionPos - pBuffer.get()));
    memcpy(pInsertionPos, str.c_str(), str.size());

    bool result;
    {
        result = originalParseArchiveTree(A1, pBuffer.get(), newSize, pDatabase);
    }

    return result;
}

bool ArchiveTreePatcher::enabled = false;

void ArchiveTreePatcher::applyPatches()
{
    if (enabled)
        return;

    enabled = true;

    INSTALL_HOOK(ParseArchiveTree);
}
