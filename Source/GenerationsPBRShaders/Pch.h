#pragma once

#define WIN32_LEAN_AND_MEAN

// Detours
#include <Windows.h>
#include <detours.h>

// DirectXTex
#ifdef ENABLE_IBL_CAPTURE_SERVICE
#include <DirectXTex.h>
#endif

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

#include <Sonic.h>
#include <Hedgehog.h>

namespace Eigen
{
	using AlignedVector3f = AlignedVector3<float>;
}

#include "Configuration.h"
#include "Math.h"
#include "SceneEffect.h"

// Other
#include <INIReader.h>

extern bool globalUsePBR;