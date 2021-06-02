#include "CpkBinder.h"
#include <Shlwapi.h>

HOOK(void, __fastcall, CFileBinderCriBindCpk, 0x66AAC0, void* This, void* Edx, const hh::base::CSharedString& filePath, int32_t priority)
{
	if (!CpkBinder::cpkPaths.empty())
	{
		for (size_t i = 0; i < CpkBinder::cpkPaths.size(); i++)
			originalCFileBinderCriBindCpk(This, Edx, CpkBinder::cpkPaths[i].c_str(), 10000 + i);

		CpkBinder::cpkPaths.clear();
	}

	originalCFileBinderCriBindCpk(This, Edx, filePath, priority);
}

bool CpkBinder::enabled = false;
std::vector<std::string> CpkBinder::cpkPaths;

void CpkBinder::applyPatches(ModInfo* info)
{
	if (enabled)
		return;

	enabled = true;

	for (auto& mod : *info->ModList)
	{
		std::string cpkPath = mod->Path;
		size_t pos = cpkPath.find_last_of("\\/");
		if (pos != std::string::npos)
			cpkPath.erase(pos + 1);

		cpkPath += "disk/mod.cpk";

		WCHAR cpkPathWideChar[1024];
		MultiByteToWideChar(CP_UTF8, 0, cpkPath.c_str(), -1, cpkPathWideChar, 1024);

		WCHAR fullCpkPathWideChar[1024];
		GetFullPathNameW(cpkPathWideChar, 1024, fullCpkPathWideChar, nullptr);

		if (!PathFileExistsW(fullCpkPathWideChar))
			continue;

		CHAR fullCpkPathMultiByte[1024];
		WideCharToMultiByte(CP_UTF8, 0, fullCpkPathWideChar, -1, fullCpkPathMultiByte, 1024, nullptr, nullptr);

		cpkPaths.emplace_back(fullCpkPathMultiByte);
	}

	INSTALL_HOOK(CFileBinderCriBindCpk);
}
