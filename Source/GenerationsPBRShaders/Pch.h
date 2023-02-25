#pragma once

// BlueBlur
#include <BlueBlur.h>

#define WIN32_LEAN_AND_MEAN

// Detours
#include <Windows.h>
#include <detours.h>
#include <wrl/client.h>

using Microsoft::WRL::ComPtr;

// DirectXTex
#ifdef ENABLE_IBL_CAPTURE_SERVICE
#include <DirectXTex.h>
#endif

// d3d9
#include <d3d9.h>

// d3d11
#include <d3d11.h> 

// std
#include <stdint.h>
#include <array>
#include <set>
#include <vector>

// HedgeLib
#include <hedgelib/hl_compression.h>
#include <hedgelib/io/hl_bina.h>
#include <hedgelib/io/hl_hh_mirage.h>
#include <hedgelib/models/hl_hh_model.h>

// LostCodeLoader
#include <LostCodeLoader.h>

// HBAOPlus
#include <GFSDK_SSAO.h>

namespace Eigen
{
	using AlignedVector3f = AlignedVector3<float>;
	using Matrix34f = Matrix<float, 3, 4>;
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

// GenerationsD3D11
#include <DllMods/Source/GenerationsD3D11/Mod/Include/GenerationsD3D11.h>

// LZ4
#include <lz4.h>

extern bool globalUsePBR;