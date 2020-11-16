#ifndef SCENE_MATERIAL_DEFAULT_HLSL_INCLUDED
#define SCENE_MATERIAL_DEFAULT_HLSL_INCLUDED

#include "../Declarations.hlsl"

#if (defined(IsChrEyeCDRF) && IsChrEyeCDRF) || \
    (defined(IsEye2) && IsEye2)

#define DECLARATION_TYPE    EyeDeclaration

#elif (defined(HasNormal) && HasNormal) || \
      (defined(IsDefault2Normal) && IsDefault2Normal)

#define DECLARATION_TYPE    NormalDeclaration

#elif defined(IsWater2) && IsWater2

#define DECLARATION_TYPE    WaterDeclaration

#else
#define DECLARATION_TYPE    DefaultDeclaration
#endif

#endif