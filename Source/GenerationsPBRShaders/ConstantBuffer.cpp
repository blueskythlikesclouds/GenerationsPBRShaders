#include "ConstantBuffer.h"

ConstantBuffer<SceneEffectCB, 2, true, false> sceneEffectCB;
ConstantBuffer<RenderDataCB, 3, true, false> renderDataCB;
ConstantBuffer<GITextureCB, 4, true, true> giTextureCB;
ConstantBuffer<FilterCB, 5, true, true> filterCB;
