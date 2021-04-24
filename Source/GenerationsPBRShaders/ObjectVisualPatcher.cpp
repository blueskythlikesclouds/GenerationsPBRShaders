#include "ObjectVisualPatcher.h"

bool ObjectVisualPatcher::enabled = false;

void ObjectVisualPatcher::applyPatches()
{
    if (enabled)
        return;

    enabled = true;

    // UpReel
    {
        WRITE_MEMORY(0x102FE32, char*, "Default2NoV_ConstTexCoord");
        WRITE_MEMORY(0x102FE75, char*, "Common2_dp@@_NoLight_NoGI_ConstTexCoord");
        WRITE_MEMORY(0x102FEB8, char*, "cmn_metal_ms_wire_HD_abd");
        WRITE_MEMORY(0x102FEF9, char*, "cmn_metal_ms_wire_HD_prm");
    }

    // Pulley
    {
        WRITE_MEMORY(0x11210AC, char*, "Common2_dp");
        WRITE_MEMORY(0x11211B4, char*, "cmn_metal_ms_wire_HD_abd");
        WRITE_MEMORY(0x112121A, char*, "cmn_metal_ms_wire_HD_prm");
        WRITE_MEMORY(0x1120685, char*, "specular");
    }
}
