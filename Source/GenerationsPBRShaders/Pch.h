#pragma once

#define WIN32_LEAN_AND_MEAN

// Detours
#include <Windows.h>
#include <detours.h>

// DirectXTex
#include <DirectXTex.h>

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

#include "Configuration.h"
#include "Math.h"
#include "SceneEffect.h"

// boost
#include <boost/pool/pool_alloc.hpp>

// Other
#include <INIReader.h>

extern bool globalUsePBR;