#include "SampleChunkResource.h"

HOOK(void, __fastcall, CSampleChunkResourceResolvePointer, 0x732E50, hl::u8* data)
{
    if (!hl::hh::mirage::has_sample_chunk_header_unfixed(data))
        return originalCSampleChunkResourceResolvePointer(data);

    hl::hh::mirage::sample_chunk::fix(data);

    hl::u32 version;
    hl::u8* contexts = hl::hh::mirage::sample_chunk::get_data<hl::u8>(data, &version);

    // Reinterpret the header as the standard sample chunk header
    // and fill relevant fields to make it transparent to the game.
    const auto header = reinterpret_cast<hl::hh::mirage::standard::raw_header*>(data);

    header->version = _byteswap_ulong(version);
    header->data = _byteswap_ulong((unsigned long)(contexts - data));
}

void setMagicAsVersion(void* data, const hl::u32 version)
{
    if (data && hl::hh::mirage::has_sample_chunk_header_unfixed(data))
        static_cast<hl::hh::mirage::sample_chunk::raw_header*>(data)->magic = _byteswap_ulong(version);
}

// These Make functions use the version number
// before calling ResolvePointer, so they
// need special handling.

HOOK(void, __cdecl, CModelDataMake, 0x7337A0, const hh::base::CSharedString& rName, uint8_t* pData, size_t length, hh::db::CDatabaseData* pDatabaseData, hh::mr::CRenderingInfrastructure* pRenderingInfrastructure)
{
    setMagicAsVersion(pData, 5);
    return originalCModelDataMake(rName, pData, length, pDatabaseData, pRenderingInfrastructure);
}

HOOK(void, __cdecl, CTerrainModelDataMake, 0x734960, const hh::base::CSharedString& rName, uint8_t* pData, size_t length, hh::db::CDatabaseData* pDatabaseData, hh::mr::CRenderingInfrastructure* pRenderingInfrastructure)
{
    setMagicAsVersion(pData, 5);
    return originalCTerrainModelDataMake(rName, pData, length, pDatabaseData, pRenderingInfrastructure);
}

void SampleChunkResource::applyPatches()
{
    INSTALL_HOOK(CSampleChunkResourceResolvePointer);
    INSTALL_HOOK(CModelDataMake);
    INSTALL_HOOK(CTerrainModelDataMake);
}
