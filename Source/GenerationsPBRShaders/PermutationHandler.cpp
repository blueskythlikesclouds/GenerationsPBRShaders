﻿#include "PermutationHandler.h"

struct Permutation
{
    const char* name;
    size_t registerIndex;
};

struct PermutedShaderFunction
{
    size_t byteSize;
    const DWORD* function;
    union
    {
        IDirect3DVertexShader9* d3dVertexShader;
        IDirect3DPixelShader9* d3dPixelShader;
    };
};

struct PermutationContainer
{
    size_t permutationCount;
    const Permutation* permutations;
    size_t* permutationMap;
    size_t shaderCount;
    PermutedShaderFunction* shaders;
};

class PermutedShader
{
public:
    ULONG refCount;
    DX_PATCH::IDirect3DDevice9* dxpDevice;

    union
    {
        void* binaryHandle;
        IDirect3DVertexShader9* d3dVertexShader;
        IDirect3DPixelShader9* d3dPixelShader;
    };

    PermutationContainer* permutationContainer;
    bool isPermuted;

    IDirect3DDevice9* d3dDevice() const
    {
        return *(IDirect3DDevice9**)((char*)dxpDevice + 8);
    }

    PermutedShader(DX_PATCH::IDirect3DDevice9* dxpDevice, const DWORD* data, const size_t dataSize, const bool isPixelShader) : refCount(1), dxpDevice(dxpDevice),
        isPermuted(hlBINAHasV2Header(data))
    {
        if (isPermuted)
        {
            binaryHandle = operator new(dataSize);
            memcpy(binaryHandle, data, dataSize);
            hlBINAV2Fix(binaryHandle, dataSize);
            permutationContainer = (PermutationContainer*)hlBINAV2GetData(binaryHandle);

            for (size_t i = 0; i < permutationContainer->shaderCount; i++)
            {
                if (isPixelShader)
                    d3dDevice()->CreatePixelShader(permutationContainer->shaders[i].function, &permutationContainer->shaders[i].d3dPixelShader);
                else
                    d3dDevice()->CreateVertexShader(permutationContainer->shaders[i].function, &permutationContainer->shaders[i].d3dVertexShader);
            }
        }
        else if (isPixelShader)
            d3dDevice()->CreatePixelShader(data, &d3dPixelShader);

        else
            d3dDevice()->CreateVertexShader(data, &d3dVertexShader);
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
        if (isPermuted)
        {
            for (size_t i = 0; i < permutationContainer->shaderCount; i++)
                permutationContainer->shaders[i].d3dVertexShader->Release(); // same vtable as d3dPixelShader

            operator delete(binaryHandle);
        }

        else
        {
            d3dVertexShader->Release(); // same vtable as d3dPixelShader
        }
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

        if (shader->isPermuted)
        {
            size_t index = 0;

            for (size_t i = 0; i < shader->permutationContainer->permutationCount; i++)
            {
                if (permutations & (1 << shader->permutationContainer->permutations[i].registerIndex))
                    index |= 1 << i;
            }

            if (isPixelShader)
                shader->d3dDevice()->SetPixelShader(shader->permutationContainer->shaders[shader->permutationContainer->permutationMap[index]].d3dPixelShader);
            else
                shader->d3dDevice()->SetVertexShader(shader->permutationContainer->shaders[shader->permutationContainer->permutationMap[index]].d3dVertexShader);
        }

        else
        {
            if (isPixelShader)
                shader->d3dDevice()->SetPixelShader(shader->d3dPixelShader);
            else
                shader->d3dDevice()->SetVertexShader(shader->d3dVertexShader);
        }

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

namespace
{
    using DxpDevice = DX_PATCH::IDirect3DDevice9;

    PermutedShaderHandler<false> vertexShaderHandler;
    PermutedShaderHandler<true> pixelShaderHandler;

    void validateShaderPermutations()
    {
        vertexShaderHandler.validate();
        pixelShaderHandler.validate();
    }

    VTABLE_HOOK(HRESULT, __fastcall, DxpDevice, DrawPrimitive, void* Edx, D3DPRIMITIVETYPE PrimitiveType, UINT StartVertex, UINT PrimitiveCount)
    {
        validateShaderPermutations();
        return originalDxpDeviceDrawPrimitive(This, Edx, PrimitiveType, StartVertex, PrimitiveCount);
    }

    VTABLE_HOOK(HRESULT, __fastcall, DxpDevice, DrawIndexedPrimitive, void* Edx, D3DPRIMITIVETYPE PrimitiveType, INT BaseVertexIndex, UINT MinVertexIndex, UINT NumVertices, UINT startIndex, UINT primCount)
    {
        validateShaderPermutations();
        return originalDxpDeviceDrawIndexedPrimitive(This, Edx, PrimitiveType, BaseVertexIndex, MinVertexIndex, NumVertices, startIndex, primCount);
    }

    VTABLE_HOOK(HRESULT, __fastcall, DxpDevice, DrawPrimitiveUP, void* Edx, D3DPRIMITIVETYPE PrimitiveType, UINT PrimitiveCount, const void* pVertexStreamZeroData, UINT VertexStreamZeroStride)
    {
        validateShaderPermutations();
        return originalDxpDeviceDrawPrimitiveUP(This, Edx, PrimitiveType, PrimitiveCount, pVertexStreamZeroData, VertexStreamZeroStride);
    }

    VTABLE_HOOK(HRESULT, __fastcall, DxpDevice, DrawIndexedPrimitiveUP, void* Edx, D3DPRIMITIVETYPE PrimitiveType, UINT MinVertexIndex, UINT NumVertices, UINT PrimitiveCount, const void* pIndexData, D3DFORMAT IndexDataFormat, const void* pVertexStreamZeroData, UINT VertexStreamZeroStride)
    {
        validateShaderPermutations();
        return originalDxpDeviceDrawIndexedPrimitiveUP(This, Edx, PrimitiveType, MinVertexIndex, NumVertices, PrimitiveCount, pIndexData, IndexDataFormat, pVertexStreamZeroData, VertexStreamZeroStride);
    }

    VTABLE_HOOK(HRESULT, __fastcall, DxpDevice, CreateVertexShader, void* Edx, const DWORD* pFunction, DX_PATCH::IDirect3DVertexShader9** ppShader, DWORD FunctionSize)
    {
        *ppShader = reinterpret_cast<DX_PATCH::IDirect3DVertexShader9*>(new PermutedShader(This, pFunction, FunctionSize, false));
        return S_OK;
    }

    VTABLE_HOOK(HRESULT, __fastcall, DxpDevice, SetVertexShader, void* Edx, DX_PATCH::IDirect3DVertexShader9* pShader)
    {
        vertexShaderHandler.setShader(reinterpret_cast<PermutedShader*>(pShader));
        return S_OK;
    }

    VTABLE_HOOK(HRESULT, __fastcall, DxpDevice, SetVertexShaderConstantB, void* Edx, UINT StartRegister, const BOOL* pConstantData, UINT BoolCount)
    {
        vertexShaderHandler.setPermutations(StartRegister, pConstantData, BoolCount);
        //return originalDxpDeviceSetVertexShaderConstantB(This, Edx, StartRegister, pConstantData, BoolCount);
        return S_OK;
    }

    VTABLE_HOOK(HRESULT, __fastcall, DxpDevice, CreatePixelShader, void* Edx, const DWORD* pFunction, DX_PATCH::IDirect3DPixelShader9** ppShader, DWORD FunctionSize)
    {
        *ppShader = reinterpret_cast<DX_PATCH::IDirect3DPixelShader9*>(new PermutedShader(This, pFunction, FunctionSize, true));
        return S_OK;
    }

    VTABLE_HOOK(HRESULT, __fastcall, DxpDevice, SetPixelShader, void* Edx, DX_PATCH::IDirect3DPixelShader9* pShader)
    {
        pixelShaderHandler.setShader(reinterpret_cast<PermutedShader*>(pShader));
        return S_OK;
    }

    VTABLE_HOOK(HRESULT, __fastcall, DxpDevice, SetPixelShaderConstantB, void* Edx, UINT StartRegister, const BOOL* pConstantData, UINT BoolCount)
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
}

void PermutationHandler::applyPatches()
{
    INSTALL_HOOK(InitializeGraphicsDevice);
}