﻿#include "GIHandler.h"

struct FixedString
{
    char* pStr;
    size_t length;

    void CopyTo(char* pBuffer) const
    {
        memcpy(pBuffer, pStr, length);
        pBuffer[length] = '\0';
    }

    bool Equals(const FixedString& right) const
    {
        return length == right.length && memcmp(pStr, right.pStr, length) == 0;
    }
};

struct Rect
{
    float width;
    float height;
    float x;
    float y;
};

struct SubTexture
{
    FixedString atlasName;
    Rect rect;
};

using MapType = hh::map<FixedString, SubTexture>;
using NodeType = MapType::node;

class GIStore : public DX_PATCH::IUnknown9
{
    ULONG refCount;

public:
    boost::shared_ptr<Hedgehog::Mirage::CPictureData> spGITex;
    boost::shared_ptr<Hedgehog::Mirage::CPictureData> spOcclusionTex;
    Rect occlusionRect;
    bool isSg;

    explicit GIStore(const boost::shared_ptr<Hedgehog::Mirage::CPictureData>& spGITex, const boost::shared_ptr<Hedgehog::Mirage::CPictureData>& spOcclusionTex, const Rect& occlusionRect, const bool isSg)
        : refCount(1), spGITex(spGITex), spOcclusionTex(spOcclusionTex), occlusionRect(occlusionRect), isSg(isSg)
    {
    }

    HRESULT QueryInterface(const IID& riid, void** ppvObj) override
    {
        (*ppvObj) = this; // specific to the hook below
        return S_OK;
    }

    ULONG AddRef() override
    {
        return InterlockedIncrement(&refCount);
    }

    ULONG Release() override
    {
        const ULONG current = InterlockedDecrement(&refCount);
        if (current == 0) delete this;
        return current;
    }

    virtual ~GIStore() = default;

    void* operator new(const size_t size)
    {
        return Hedgehog::Base::fpOperatorNew(size);
    }

    void operator delete(void* pData)
    {
        Hedgehog::Base::fpOperatorDelete(pData);
    }
};

FUNCTION_PTR(NodeType*, __thiscall, fpFindAtlasSubTexture, 0x72AEE0, MapType* This, const FixedString& key);

uint32_t pMovePictureDataMidAsmHookReturnAddress = 0x728F78;

void __stdcall fMovePictureDataSetupGIStore(char* pName, MapType* pMap,
    Hedgehog::Mirage::CMirageDatabaseWrapper* pDatabaseWrapper, boost::shared_ptr<Hedgehog::Mirage::CPictureData>& spPictureData)
{
    spPictureData->Validate();

    // Create the name of the corresponding occlusion texture.
    char* pSrcSuffix = strstr(pName, "-level");
    char* pDstSuffix = strstr(pName, "_sg-level");

    const bool isSg = pDstSuffix != nullptr;

    if (pDstSuffix == nullptr)
        pDstSuffix = pSrcSuffix;

    memmove(pDstSuffix + 10, pSrcSuffix, 7);
    memcpy(pDstSuffix, "_occlusion", 10);
    pDstSuffix[17] = '\0';

    // Try to find it in the map.
    boost::shared_ptr<Hedgehog::Mirage::CPictureData> spOcclusionTex;
    Rect occlusionRect { 1, 1, 0, 0 };

    NodeType* pOcclusionNode = fpFindAtlasSubTexture(pMap, { pName, strlen(pName) });
    if (pOcclusionNode != pMap->end())
    {
        char atlasName[256];
        pOcclusionNode->m_Value.m_Value.atlasName.CopyTo(atlasName);

        pDatabaseWrapper->GetPictureData(spOcclusionTex, atlasName, 0);

        if (spOcclusionTex != nullptr)
        {
            spOcclusionTex->Validate();
            occlusionRect = pOcclusionNode->m_Value.m_Value.rect;
        }
    }

    // Crackheadedly create a stub for presenting the data to the game.
    uint8_t* pStubData = (uint8_t*)Hedgehog::Base::fpOperatorNew(sizeof(Hedgehog::Mirage::CPictureData));

    *(uint8_t*)(pStubData + offsetof(Hedgehog::Mirage::CPictureData, m_Flags)) = spPictureData->m_Flags;

    *(GIStore**)(pStubData + offsetof(Hedgehog::Mirage::CPictureData, m_pD3DTexture)) = 
        new GIStore(spPictureData, spOcclusionTex, occlusionRect, isSg);

    spPictureData = boost::shared_ptr<Hedgehog::Mirage::CPictureData>((Hedgehog::Mirage::CPictureData*)pStubData, 
        [](Hedgehog::Mirage::CPictureData* pMem)
        {
            pMem->m_pD3DTexture->Release();
            Hedgehog::Base::fpOperatorDelete(pMem);
        });
}

void __declspec(naked) fMovePictureDataMidAsmHook()
{
    __asm
    {
        // spPictureData
        lea eax, [esp + 0x0 + 0x6C]
        push eax

        // pDatabaseWrapper
        lea eax, [esp + 0x4 + 0x60]
        push eax

        // pMap
        push [esp + 0x8 + 0x30]

        // pName
        lea eax, [esp + 0xC + 0x74]
        push eax

        call fMovePictureDataSetupGIStore

        // Return
        lea ecx, [esp + 0x6C]
        push ecx
        lea ecx, [esp + 0x38]
        jmp [pMovePictureDataMidAsmHookReturnAddress]
    }
}

uint32_t pFindAtlasSubTextureMidAsmHookReturnAddress = 0x728E9E;

void __stdcall fFindAtlasSubTextureAppendSgSuffix(FixedString& fixedString)
{
    char* pSuffix = strstr(fixedString.pStr, "-level");
    memmove(pSuffix + 3, pSuffix, 7);
    memcpy(pSuffix, "_sg", 3);
    pSuffix[10] = '\0';
    fixedString.length += 3;
}

void __declspec(naked) fFindAtlasSubTextureMidAsmHook()
{
    __asm
    {
        mov edi, [edi + 4] // End of sub-texture map.
        mov[esp + 0x5C], eax // Find result.
        cmp eax, edi // Compare if find was unsuccessful.
        jnz end // Jump if not.

        // Append _sg suffix to sub-texture name and try finding that.
        lea eax, [esp + 0x40]
        push eax
        call fFindAtlasSubTextureAppendSgSuffix

        mov ecx, [esp + 0x30]
        lea eax, [esp + 0x40]
        push eax
        call fpFindAtlasSubTexture

        // Return
        mov edi, [esp + 0x30]
        mov edi, [edi + 4]
        mov[esp + 0x5C], eax
        cmp eax, edi

    end:
        jmp[pFindAtlasSubTextureMidAsmHookReturnAddress]
    }
}

HOOK(NodeType*, __fastcall, FindAtlasSubTexture, fpFindAtlasSubTexture, MapType* This, void* Edx, const FixedString& key)
{
    // Always return end in invalid returns to make my life easier.
    NodeType* result = originalFindAtlasSubTexture(This, Edx, key);
    return result != This->end() && !key.Equals(result->m_Value.m_Key) ? This->end() : result;
}

HOOK(void, __fastcall, CRenderingDeviceSetAtlasParameterData, Hedgehog::Mirage::fpCRenderingDeviceSetAtlasParameterData,
    Hedgehog::Mirage::CRenderingDevice* This, void* Edx, float* const pData)
{
    if (pData == (float*)0x13DEAB0)
    {
        originalCRenderingDeviceSetAtlasParameterData(This, Edx, pData);
        return;
    }

    DX_PATCH::IDirect3DTexture9* pDxpTex = *(DX_PATCH::IDirect3DTexture9**)((uint32_t)pData + 16);

    static GIStore* pPrevGIStore = nullptr;

    GIStore* pGIStore = nullptr;
    pDxpTex->QueryInterface(IID(), (void**)&pGIStore);

    if (pPrevGIStore != nullptr && pGIStore == pPrevGIStore)
        return;

    pPrevGIStore = pGIStore;

    const BOOL isSg = pGIStore != nullptr && pGIStore->isSg;
    const BOOL hasOcclusion = pGIStore != nullptr && pGIStore->spOcclusionTex != nullptr;

    if (hasOcclusion)
    {
        This->m_pD3DDevice->SetTexture(9, pGIStore->spOcclusionTex->m_pD3DTexture);
        This->m_pD3DDevice->SetPixelShaderConstantF(108, (const float*)&pGIStore->occlusionRect, 1);
    }

    This->m_pD3DDevice->SetTexture(10, pGIStore != nullptr ? pGIStore->spGITex->m_pD3DTexture : nullptr);

    float giParam[] = { pData[0], pData[1], pData[2], pData[3] };

    if (isSg)
    {
        giParam[0] /= 2.0f;
        giParam[1] /= 2.0f;
    }

    This->m_pD3DDevice->SetVertexShaderConstantF(186, giParam, 1);
    This->m_pD3DDevice->SetPixelShaderConstantF(107, giParam, 1);

    if (!hasOcclusion)
        This->m_pD3DDevice->SetPixelShaderConstantF(108, pData, 1);

    This->m_pD3DDevice->SetPixelShaderConstantB(9, &isSg, 1);
    This->m_pD3DDevice->SetPixelShaderConstantB(10, &hasOcclusion, 1);
}

bool GIHandler::enabled = false;

void GIHandler::applyPatches()
{
    if (enabled)
        return;

    enabled = true;

    WRITE_JUMP(0x728F73, fMovePictureDataMidAsmHook);
    WRITE_JUMP(0x728E95, fFindAtlasSubTextureMidAsmHook);

    // Don't set GI texture, we are going to do it ourselves.
    WRITE_MEMORY(0x7145CB, uint8_t, 0xEB);
    WRITE_MEMORY(0x714C07, uint8_t, 0xEB);
    WRITE_MEMORY(0x714E45, uint8_t, 0xEB);
    WRITE_MEMORY(0x715422, uint8_t, 0xEB);
    WRITE_MEMORY(0x71563E, uint8_t, 0xEB);
    WRITE_MEMORY(0x715A47, uint8_t, 0xEB);
    WRITE_MEMORY(0x715BCA, uint8_t, 0xEB);
    WRITE_MEMORY(0x715F52, uint8_t, 0xEB);

    INSTALL_HOOK(FindAtlasSubTexture);

    INSTALL_HOOK(CRenderingDeviceSetAtlasParameterData);
}
