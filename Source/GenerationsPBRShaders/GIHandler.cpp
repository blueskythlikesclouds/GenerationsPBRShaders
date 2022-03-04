#include "GIHandler.h"

#include "ConstantBuffer.h"

struct FixedString
{
    char* str;
    size_t length;

    void copyTo(char* buffer) const
    {
        memcpy(buffer, str, length);
        buffer[length] = '\0';
    }

    bool operator<(const FixedString& other) const
    {
        FUNCTION_PTR(bool, __thiscall, operatorLess, 0x72AE40, const FixedString* This, const FixedString& other);
        return operatorLess(this, other);
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

FUNCTION_PTR(void, __thiscall, findAtlasSubTexture, 0x72AEE0, MapType* This, const FixedString& key);

uint32_t movePictureDataMidAsmHookReturnAddress = 0x728F78;

void __stdcall movePictureDataSetupGIStore(char* name, MapType* map,
    hh::mr::CMirageDatabaseWrapper* databaseWrapper, boost::shared_ptr<hh::mr::CPictureData>& pictureData)
{
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

    const auto occlusionNode = map->find({ name, strlen(name) });
    if (occlusionNode != map->end())
    {
        char atlasName[256];
        occlusionNode->second.atlasName.copyTo(atlasName);

        databaseWrapper->GetPictureData(occlusionTex, atlasName, 0);

        if (occlusionTex != nullptr)
            occlusionRect = occlusionNode->second.rect;
    }

    // Create a stub for presenting the data to the game.
    auto stub = boost::make_shared<hh::mr::CPictureData>();
    stub->m_Flags = hh::db::eDatabaseDataFlags_IsMadeOne | hh::db::eDatabaseDataFlags_IsMadeAll;
    stub->m_pD3DTexture = reinterpret_cast<DX_PATCH::IDirect3DBaseTexture9*>(new GIStore(pictureData, occlusionTex, occlusionRect, isSg));

    pictureData = std::move(stub);
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

HOOK(void*, __fastcall, FindAtlasSubTexture, findAtlasSubTexture, MapType* This, void* Edx, const FixedString& key)
{
    // This is for making my life easier in the hooks above.
    // I have to do this ugly reinterpret_cast due to not being able to
    // return a structure in the function and keeping the same signature.
    auto it = This->find(key);
    return reinterpret_cast<void*&>(it);
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

    GIStore* giStore = nullptr;
    if (dxpTex)
        dxpTex->QueryInterface(IID(), (void**)&giStore);

    memcpy(&giTextureCB.giAtlasParam, pData, sizeof(Eigen::Vector4f));
    giTextureCB.isSg = giStore && giStore->isSg;
    giTextureCB.hasOcclusion = giStore && giStore->occlusionTex;

    if (giTextureCB.hasOcclusion)
    {
        This->m_pD3DDevice->SetTexture(18, giStore->occlusionTex->m_pD3DTexture);
        memcpy(&giTextureCB.occlusionAtlasParam, &giStore->occlusionRect, sizeof(Eigen::Vector4f));
    }

    giTextureCB.upload(This->m_pD3DDevice);

    This->m_pD3DDevice->SetTexture(giTextureCB.isSg ? 22 : 10, giStore ? giStore->giTex->m_pD3DTexture : nullptr);
    This->m_pD3DDevice->SetVertexShaderConstantF(186, pData, 1);
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
