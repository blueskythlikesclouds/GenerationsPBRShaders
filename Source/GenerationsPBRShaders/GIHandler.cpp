#include "GIHandler.h"

struct FixedString
{
    char* str;
    size_t length;

    void copyTo(char* buffer) const
    {
        memcpy(buffer, str, length);
        buffer[length] = '\0';
    }

    bool equals(const FixedString& right) const
    {
        return length == right.length && memcmp(str, right.str, length) == 0;
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
    boost::shared_ptr<hh::mr::CPictureData> giTex;
    boost::shared_ptr<hh::mr::CPictureData> occlusionTex;
    Rect occlusionRect;
    bool isSg;

    explicit GIStore(const boost::shared_ptr<hh::mr::CPictureData>& giTex, const boost::shared_ptr<hh::mr::CPictureData>& occlusionTex, const Rect& occlusionRect, const bool isSg)
        : refCount(1), giTex(giTex), occlusionTex(occlusionTex), occlusionRect(occlusionRect), isSg(isSg)
    {
    }

    HRESULT QueryInterface(const IID& riid, void** ppvObj) override
    {
        (*ppvObj) = this; // specific to the hook below
        return S_OK;
    }

    ULONG AddRef() override
    {
        return ++refCount;
    }

    ULONG Release() override
    {
        const ULONG current = --refCount;
        if (current == 0) delete this;
        return current;
    }

    virtual ~GIStore() = default;
};

FUNCTION_PTR(NodeType*, __thiscall, findAtlasSubTexture, 0x72AEE0, MapType* This, const FixedString& key);

uint32_t movePictureDataMidAsmHookReturnAddress = 0x728F78;

void __stdcall movePictureDataSetupGIStore(char* name, MapType* map,
    hh::mr::CMirageDatabaseWrapper* databaseWrapper, boost::shared_ptr<hh::mr::CPictureData>& pictureData)
{
    pictureData->Validate();

    // Create the name of the corresponding occlusion texture.
    char* srcSuffix = strstr(name, "-level");
    char* dstSuffix = strstr(name, "_sg-level");

    const bool isSg = dstSuffix != nullptr;

    if (dstSuffix == nullptr)
        dstSuffix = srcSuffix;

    memmove(dstSuffix + 10, srcSuffix, 7);
    memcpy(dstSuffix, "_occlusion", 10);
    dstSuffix[17] = '\0';

    // Try to find it in the map.
    boost::shared_ptr<hh::mr::CPictureData> occlusionTex;
    Rect occlusionRect { 1, 1, 0, 0 };

    NodeType* occlusionNode = findAtlasSubTexture(map, { name, strlen(name) });
    if (occlusionNode != map->end())
    {
        char atlasName[256];
        occlusionNode->m_Value.m_Value.atlasName.copyTo(atlasName);

        databaseWrapper->GetPictureData(occlusionTex, atlasName, 0);

        if (occlusionTex != nullptr)
        {
            occlusionTex->Validate();
            occlusionRect = occlusionNode->m_Value.m_Value.rect;
        }
    }

    // Crackheadedly create a stub for presenting the data to the game.
    uint8_t* stubData = (uint8_t*)operator new(sizeof(hh::mr::CPictureData));
    memset(stubData, 0, sizeof(hh::mr::CPictureData));

    *(uint8_t*)(stubData + offsetof(hh::mr::CPictureData, m_Flags)) = 0x3;

    *(GIStore**)(stubData + offsetof(hh::mr::CPictureData, m_pD3DTexture)) = 
        new GIStore(pictureData, occlusionTex, occlusionRect, isSg);

    pictureData = boost::shared_ptr<hh::mr::CPictureData>((hh::mr::CPictureData*)stubData,
        [](hh::mr::CPictureData* mem)
        {
            mem->m_pD3DTexture->Release();
            operator delete (mem);
        });
}

void __declspec(naked) movePictureDataMidAsmHook()
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

        call movePictureDataSetupGIStore

        // Return
        lea ecx, [esp + 0x6C]
        push ecx
        lea ecx, [esp + 0x38]
        jmp [movePictureDataMidAsmHookReturnAddress]
    }
}

uint32_t findAtlasSubTextureMidAsmHookReturnAddress = 0x728E9E;

void __fastcall findAtlasSubTextureAppendSgSuffix(FixedString& fixedString)
{
    char* suffix = strstr(fixedString.str, "-level");
    memmove(suffix + 3, suffix, 7);
    memcpy(suffix, "_sg", 3);
    suffix[10] = '\0';
    fixedString.length += 3;
}

void __fastcall findAtlasSubTextureRemoveSgSuffix(FixedString& fixedString)
{
    char* suffix = strstr(fixedString.str, "-level");
    memmove(suffix - 3, suffix, 7);
    suffix[4] = '\0';
    fixedString.length -= 3;
}

void __declspec(naked) findAtlasSubTextureMidAsmHook()
{
    __asm
    {
        // TODO: Make this hook shorter

        mov edi, [edi + 4] // End of sub-texture map.
        mov[esp + 0x5C], eax // Find result.
        cmp eax, edi // Compare if find was unsuccessful.
        jnz end // Jump if not.

        // Append _sg suffix to sub-texture name and try finding that.
        lea ecx, [esp + 0x40]
        call findAtlasSubTextureAppendSgSuffix

        mov ecx, [esp + 0x30]
        lea eax, [esp + 0x40]
        push eax
        call findAtlasSubTexture

        // Remove _sg suffix if we couldn't locate it
        // so rest of the function can execute properly.
        mov edi, [esp + 0x30]
        mov edi, [edi + 4]
        mov[esp + 0x5C], eax
        cmp eax, edi
        jnz return

        lea ecx, [esp + 0x40]
        call findAtlasSubTextureRemoveSgSuffix

        // Return
    return:
        mov edi, [esp + 0x30]
        mov edi, [edi + 4]
        mov eax, [esp + 0x5C]
        cmp eax, edi

    end:
        jmp[findAtlasSubTextureMidAsmHookReturnAddress]
    }
}

HOOK(NodeType*, __fastcall, FindAtlasSubTexture, findAtlasSubTexture, MapType* This, void* Edx, const FixedString& key)
{
    // Always return end in invalid returns to make my life easier.
    NodeType* result = originalFindAtlasSubTexture(This, Edx, key);
    return result != This->end() && !key.equals(result->m_Value.m_Key) ? This->end() : result;
}

HOOK(void, __fastcall, CRenderingDeviceSetAtlasParameterData, hh::mr::fpCRenderingDeviceSetAtlasParameterData,
    hh::mr::CRenderingDevice* This, void* Edx, float* const pData)
{
    if (pData == (float*)0x13DEAB0)
    {
        originalCRenderingDeviceSetAtlasParameterData(This, Edx, pData);
        return;
    }

    DX_PATCH::IDirect3DTexture9* dxpTex = *(DX_PATCH::IDirect3DTexture9**)((uint32_t)pData + 16);

    static GIStore* prevGIStore = nullptr;

    GIStore* giStore = nullptr;
    dxpTex->QueryInterface(IID(), (void**)&giStore);

    if (prevGIStore != nullptr && giStore == prevGIStore)
        return;

    prevGIStore = giStore;

    const BOOL isSg = giStore != nullptr && giStore->isSg;
    const BOOL hasOcclusion = giStore != nullptr && giStore->occlusionTex != nullptr;

    if (hasOcclusion)
    {
        This->m_pD3DDevice->SetTexture(9, giStore->occlusionTex->m_pD3DTexture);
        This->m_pD3DDevice->SetPixelShaderConstantF(111, (const float*)&giStore->occlusionRect, 1);
    }

    This->m_pD3DDevice->SetTexture(10, giStore != nullptr ? giStore->giTex->m_pD3DTexture : nullptr);

    float giParam[] = { pData[0], pData[1], pData[2], pData[3] };

    if (isSg)
    {
        giParam[0] /= 2.0f;
        giParam[1] /= 2.0f;
    }

    This->m_pD3DDevice->SetVertexShaderConstantF(186, giParam, 1);
    This->m_pD3DDevice->SetPixelShaderConstantF(110, giParam, 1);

    if (!hasOcclusion)
        This->m_pD3DDevice->SetPixelShaderConstantF(111, pData, 1);

    This->m_pD3DDevice->SetPixelShaderConstantB(8, &isSg, 1);
    This->m_pD3DDevice->SetPixelShaderConstantB(9, &hasOcclusion, 1);
}

bool GIHandler::enabled = false;

void GIHandler::applyPatches()
{
    if (enabled)
        return;

    enabled = true;

    WRITE_JUMP(0x728F73, movePictureDataMidAsmHook);
    WRITE_JUMP(0x728E95, findAtlasSubTextureMidAsmHook);

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
