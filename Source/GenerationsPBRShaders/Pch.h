#pragma once

#define WIN32_LEAN_AND_MEAN

// Detours
#include <Windows.h>
#include <detours.h>

// DirectXTex
#ifdef ENABLE_IBL_CAPTURE_SERVICE
#include <DirectXTex.h>
#endif

// d3d9
#include <d3d9.h>

// std
#include <stdint.h>
#include <array>
#include <set>
#include <vector>

// HedgeLib
#include <hedgelib/hl_compression.h>
#include <hedgelib/io/hl_bina.h>

// LostCodeLoader
#include <LostCodeLoader.h>

// BlueBlur
#include <BlueBlur.h>

namespace Eigen
{
	using AlignedVector3f = AlignedVector3<float>;
}

#include "Configuration.h"
#include "Math.h"
#include "SceneEffect.h"

// Other
#include <INIReader.h>
#include <Helpers.h>

// Parameter Editor
#define DEBUG_DRAW_TEXT_DLL_IMPORT
#include <DllMods/Source/GenerationsParameterEditor/Include/DebugDrawText.h>

extern bool globalUsePBR;