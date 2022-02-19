#include "PermutationHandler.h"

using DxpDevice = DX_PATCH::IDirect3DDevice9;

VTABLE_HOOK(HRESULT, __fastcall, DxpDevice, DrawPrimitive, void* Edx, D3DPRIMITIVETYPE PrimitiveType, UINT StartVertex, UINT PrimitiveCount);
VTABLE_HOOK(HRESULT, __fastcall, DxpDevice, DrawIndexedPrimitive, void* Edx, D3DPRIMITIVETYPE PrimitiveType, INT BaseVertexIndex, UINT MinVertexIndex, UINT NumVertices, UINT startIndex, UINT primCount);
VTABLE_HOOK(HRESULT, __fastcall, DxpDevice, DrawPrimitiveUP, void* Edx, D3DPRIMITIVETYPE PrimitiveType, UINT PrimitiveCount, const void* pVertexStreamZeroData, UINT VertexStreamZeroStride);
VTABLE_HOOK(HRESULT, __fastcall, DxpDevice, DrawIndexedPrimitiveUP, void* Edx, D3DPRIMITIVETYPE PrimitiveType, UINT MinVertexIndex, UINT NumVertices, UINT PrimitiveCount, const void* pIndexData, D3DFORMAT IndexDataFormat, const void* pVertexStreamZeroData, UINT VertexStreamZeroStride);
VTABLE_HOOK(HRESULT, __fastcall, DxpDevice, CreateVertexShader, void* Edx, const DWORD* pFunction, DX_PATCH::IDirect3DVertexShader9** ppShader, DWORD FunctionSize);
VTABLE_HOOK(HRESULT, __fastcall, DxpDevice, SetVertexShader, void* Edx, DX_PATCH::IDirect3DVertexShader9* pShader);
VTABLE_HOOK(HRESULT, __fastcall, DxpDevice, SetVertexShaderConstantB, void* Edx, UINT StartRegister, const BOOL* pConstantData, UINT BoolCount);
VTABLE_HOOK(HRESULT, __fastcall, DxpDevice, CreatePixelShader, void* Edx, const DWORD* pFunction, DX_PATCH::IDirect3DPixelShader9** ppShader, DWORD FunctionSize);
VTABLE_HOOK(HRESULT, __fastcall, DxpDevice, SetPixelShader, void* Edx, DX_PATCH::IDirect3DPixelShader9* pShader);
VTABLE_HOOK(HRESULT, __fastcall, DxpDevice, SetPixelShaderConstantB, void* Edx, UINT StartRegister, const BOOL* pConstantData, UINT BoolCount);

class PermutedShader
{
public:
    ULONG refCount;
    DX_PATCH::IDirect3DDevice9* dxpDevice;

    uint8_t* metadata;
    std::vector<DX_PATCH::IUnknown9*> d3dShaders;

    PermutedShader(DX_PATCH::IDirect3DDevice9* dxpDevice, char* data, const size_t dataSize, const bool isPixelShader) : refCount(1), dxpDevice(dxpDevice), metadata(nullptr)
    {
        const bool hasMetadata = *(int*)data != 0xFFFF0300 && *(int*)data != 0xFFFE0300;

        if (hasMetadata)
        {
            const uint8_t boolCount = *(uint8_t*)data;
            d3dShaders.reserve(1 << boolCount);

            const size_t metadataSize = 1 + boolCount + (1 << boolCount);
            metadata = (uint8_t*)malloc(metadataSize);
            memcpy(metadata, data, metadataSize);

            for (size_t i = (metadataSize + 3) & ~3; i < dataSize;)
            {
                const uint32_t functionSize = *(uint32_t*)(data + i);
                const DWORD* function = (const DWORD*)(data + i + 4);

                DX_PATCH::IUnknown9* d3dShader = nullptr;

                if (isPixelShader)
                    originalDxpDeviceCreatePixelShader(dxpDevice, nullptr, function, (DX_PATCH::IDirect3DPixelShader9**)&d3dShader, functionSize);
                else
                    originalDxpDeviceCreateVertexShader(dxpDevice, nullptr, function, (DX_PATCH::IDirect3DVertexShader9**)&d3dShader, functionSize);

                d3dShaders.push_back(d3dShader);

                i += 4 + functionSize;
            }
        }
        else
        {
            DX_PATCH::IUnknown9* d3dShader = nullptr;

            if (isPixelShader)
                originalDxpDeviceCreatePixelShader(dxpDevice, nullptr, (const DWORD*)data, (DX_PATCH::IDirect3DPixelShader9**)&d3dShader, dataSize);
            else
                originalDxpDeviceCreateVertexShader(dxpDevice, nullptr, (const DWORD*)data, (DX_PATCH::IDirect3DVertexShader9**)&d3dShader, dataSize);

            d3dShaders.push_back(d3dShader);
        }

        d3dShaders.shrink_to_fit();
    }

    virtual HRESULT QueryInterface(REFIID riid, void** ppvObj)
    {
        *ppvObj = this;
        return S_OK;
    }

    virtual ULONG AddRef()
    {
        return ++refCount;
    }

    virtual ULONG Release()
    {
        const ULONG result = --refCount;
        if (result == 0)
            delete this;

        return result;
    }

    virtual ~PermutedShader()
    {
        free(metadata);

        for (auto& shader : d3dShaders)
            shader->Release();
    }

    virtual HRESULT GetDevice(DX_PATCH::IDirect3DDevice9** ppDevice)
    {
        *ppDevice = dxpDevice;
        (*ppDevice)->AddRef();
        return S_OK;
    }

    virtual HRESULT GetFunction(void*, UINT* pSizeOfData)
    {
        // We have multiple!
        return E_FAIL;
    }
};

template <bool isPixelShader>
class PermutedShaderHandler
{
public:
    PermutedShader* shader;
    size_t permutations;
    bool dirty;

    PermutedShaderHandler() : shader(nullptr), permutations(0), dirty(true) {}

    void validate()
    {
        if (!dirty || !shader)
            return;

        DX_PATCH::IUnknown9* d3dShader;

        if (shader->metadata)
        {
            size_t index = 0;

            const uint8_t boolCount = shader->metadata[0];
            for (int i = 0; i < boolCount; i++)
            {
                if (permutations & (1 << shader->metadata[1 + i]))
                    index |= 1 << i;
            }

            d3dShader = shader->d3dShaders[shader->metadata[1 + boolCount + index]];
        }

        else
            d3dShader = shader->d3dShaders[0];

        if (isPixelShader)
            originalDxpDeviceSetPixelShader(shader->dxpDevice, nullptr, (DX_PATCH::IDirect3DPixelShader9*)d3dShader);

        else
            originalDxpDeviceSetVertexShader(shader->dxpDevice, nullptr, (DX_PATCH::IDirect3DVertexShader9*)d3dShader);

        dirty = false;
    }

    void setShader(PermutedShader* srcShader)
    {
        if (shader == srcShader)
            return;

        if (shader)
            shader->Release();

        shader = srcShader;

        if (shader)
            shader->AddRef();

        dirty = true;
    }

    void setPermutations(const size_t srcPermutations)
    {
        if (permutations == srcPermutations)
            return;

        permutations = srcPermutations;
        dirty = true;
    }

    void setPermutations(const UINT startRegister, const BOOL* constantData, UINT boolCount)
    {
        const size_t oldPermutations = permutations;

        for (UINT i = 0; i < boolCount; i++)
        {
            const UINT index = startRegister + i;

            if (constantData[i])
                permutations |= 1 << index;
            else
                permutations &= ~(1 << index);
        }

        if (oldPermutations != permutations)
            dirty = true;
    }
};

PermutedShaderHandler<false> vertexShaderHandler;
PermutedShaderHandler<true> pixelShaderHandler;

void validateShaderPermutations()
{
    vertexShaderHandler.validate();
    pixelShaderHandler.validate();
}

HRESULT __fastcall implOfDxpDeviceDrawPrimitive(DxpDevice* This, void* Edx, D3DPRIMITIVETYPE PrimitiveType, UINT StartVertex, UINT PrimitiveCount)
{
    validateShaderPermutations();
    return originalDxpDeviceDrawPrimitive(This, Edx, PrimitiveType, StartVertex, PrimitiveCount);
}

HRESULT __fastcall implOfDxpDeviceDrawIndexedPrimitive(DxpDevice* This, void* Edx, D3DPRIMITIVETYPE PrimitiveType, INT BaseVertexIndex, UINT MinVertexIndex, UINT NumVertices, UINT startIndex, UINT primCount)
{
    validateShaderPermutations();
    return originalDxpDeviceDrawIndexedPrimitive(This, Edx, PrimitiveType, BaseVertexIndex, MinVertexIndex, NumVertices, startIndex, primCount);
}

HRESULT __fastcall implOfDxpDeviceDrawPrimitiveUP(DxpDevice* This, void* Edx, D3DPRIMITIVETYPE PrimitiveType, UINT PrimitiveCount, const void* pVertexStreamZeroData, UINT VertexStreamZeroStride)
{
    validateShaderPermutations();
    return originalDxpDeviceDrawPrimitiveUP(This, Edx, PrimitiveType, PrimitiveCount, pVertexStreamZeroData, VertexStreamZeroStride);
}

HRESULT __fastcall implOfDxpDeviceDrawIndexedPrimitiveUP(DxpDevice* This, void* Edx, D3DPRIMITIVETYPE PrimitiveType, UINT MinVertexIndex, UINT NumVertices, UINT PrimitiveCount, const void* pIndexData, D3DFORMAT IndexDataFormat, const void* pVertexStreamZeroData, UINT VertexStreamZeroStride)
{
    validateShaderPermutations();
    return originalDxpDeviceDrawIndexedPrimitiveUP(This, Edx, PrimitiveType, MinVertexIndex, NumVertices, PrimitiveCount, pIndexData, IndexDataFormat, pVertexStreamZeroData, VertexStreamZeroStride);
}

HRESULT __fastcall implOfDxpDeviceCreateVertexShader(DxpDevice* This, void* Edx, const DWORD* pFunction, DX_PATCH::IDirect3DVertexShader9** ppShader, DWORD FunctionSize)
{
    *ppShader = reinterpret_cast<DX_PATCH::IDirect3DVertexShader9*>(new PermutedShader(This, (char*)pFunction, FunctionSize, false));
    return S_OK;
}

HRESULT __fastcall implOfDxpDeviceSetVertexShader(DxpDevice* This, void* Edx, DX_PATCH::IDirect3DVertexShader9* pShader)
{
    vertexShaderHandler.setShader(reinterpret_cast<PermutedShader*>(pShader));
    return S_OK;
}

HRESULT __fastcall implOfDxpDeviceSetVertexShaderConstantB(DxpDevice* This, void* Edx, UINT StartRegister, const BOOL* pConstantData, UINT BoolCount)
{
    vertexShaderHandler.setPermutations(StartRegister, pConstantData, BoolCount);

    // TODO: Get rid of this when you fix vertex shader translation
    if (StartRegister == 0)
        return originalDxpDeviceSetVertexShaderConstantB(This, Edx, StartRegister, pConstantData, BoolCount);

    return S_OK;
}

HRESULT __fastcall implOfDxpDeviceCreatePixelShader(DxpDevice* This, void* Edx, const DWORD* pFunction, DX_PATCH::IDirect3DPixelShader9** ppShader, DWORD FunctionSize)
{
    *ppShader = reinterpret_cast<DX_PATCH::IDirect3DPixelShader9*>(new PermutedShader(This, (char*)pFunction, FunctionSize, true));
    return S_OK;
}

HRESULT __fastcall implOfDxpDeviceSetPixelShader(DxpDevice* This, void* Edx, DX_PATCH::IDirect3DPixelShader9* pShader)
{
    pixelShaderHandler.setShader(reinterpret_cast<PermutedShader*>(pShader));
    return S_OK;
}

HRESULT __fastcall implOfDxpDeviceSetPixelShaderConstantB(DxpDevice* This, void* Edx, UINT StartRegister, const BOOL* pConstantData, UINT BoolCount)
{
    pixelShaderHandler.setPermutations(StartRegister, pConstantData, BoolCount);
    //return originalDxpDeviceSetPixelShaderConstantB(This, Edx, StartRegister, pConstantData, BoolCount);
    return S_OK;
}

HOOK(bool, __fastcall, InitializeGraphicsDevice, 0x6F53E0, void* This, void* Edx, void* A2)
{
    const bool success = originalInitializeGraphicsDevice(This, Edx, A2);
    if (!success)
        return false;

    DX_PATCH::IDirect3DDevice9* dxpDevice = *(DX_PATCH::IDirect3DDevice9**)((char*)This + 100);

    INSTALL_VTABLE_HOOK(DxpDevice, dxpDevice, DrawPrimitive, 82);
    INSTALL_VTABLE_HOOK(DxpDevice, dxpDevice, DrawIndexedPrimitive, 83);
    INSTALL_VTABLE_HOOK(DxpDevice, dxpDevice, DrawPrimitiveUP, 84);
    INSTALL_VTABLE_HOOK(DxpDevice, dxpDevice, DrawIndexedPrimitiveUP, 85);

    INSTALL_VTABLE_HOOK(DxpDevice, dxpDevice, CreateVertexShader, 92);
    INSTALL_VTABLE_HOOK(DxpDevice, dxpDevice, SetVertexShader, 93);
    INSTALL_VTABLE_HOOK(DxpDevice, dxpDevice, SetVertexShaderConstantB, 99);

    INSTALL_VTABLE_HOOK(DxpDevice, dxpDevice, CreatePixelShader, 107);
    INSTALL_VTABLE_HOOK(DxpDevice, dxpDevice, SetPixelShader, 108);
    INSTALL_VTABLE_HOOK(DxpDevice, dxpDevice, SetPixelShaderConstantB, 114);

    return true;
}

void PermutationHandler::applyPatches()
{
    INSTALL_HOOK(InitializeGraphicsDevice);
}
