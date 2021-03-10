#pragma once

#define WIN32_LEAN_AND_MEAN

// Detours
#include <Windows.h>
#include <detours.h>

// std
#include <stdint.h>
#include <array>
#include <set>
#include <vector>

// HedgeLib
#include <hedgelib/hl_compression.h>
#include <hedgelib/io/hl_bina.h>

// BlueBlur
#include <BlueBlur.h>

#include <Sonic.h>
#include <Hedgehog.h>

#include "Frustum.h"
#include "SceneEffect.h"

// boost
#include <boost/pool/pool_alloc.hpp>