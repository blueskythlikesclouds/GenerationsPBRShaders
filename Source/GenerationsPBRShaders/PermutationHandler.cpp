#include "PermutationHandler.h"

//#define PERMUTATION_HANDLER_LAZY_LOADING

struct Permutation
{
    const char* name;
    size_t registerIndex;
};

struct PermutedShaderFunctionHandle
{
    const DWORD* function;
    union
    {
        IDirect3DVertexShader9* d3dVertexShader;
        IDirect3DPixelShader9* d3dPixelShader;
    };

    void createVertexShader(IDirect3DDevice9* device)
    {
        device->CreateVertexShader(function, &d3dVertexShader);
    }

    IDirect3DVertexShader9* getVertexShader(IDirect3DDevice9* device)
    {
#ifdef PERMUTATION_HANDLER_LAZY_LOADING
        if (!d3dVertexShader)
            createVertexShader(device);
#endif

        return d3dVertexShader;
    }

    void createPixelShader(IDirect3DDevice9* device)
    {
        device->CreatePixelShader(function, &d3dPixelShader);
    }

    IDirect3DPixelShader9* getPixelShader(IDirect3DDevice9* device)
    {
#ifdef PERMUTATION_HANDLER_LAZY_LOADING
        if (!d3dPixelShader)
            createPixelShader(device);
#endif

        return d3dPixelShader;
    }

    void release() const
    {
        if (d3dVertexShader)
            d3dVertexShader->Release();
    }
};

struct PermutedShaderFunction
{
    size_t byteSize;
    PermutedShaderFunctionHandle handle;
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
        PermutedShaderFunctionHandle handle;
        struct
        {
            void* memory;
            PermutationContainer* container;
        };
    };

    bool isPermuted;

    IDirect3DDevice9* d3dDevice() const
    {
        return *(IDirect3DDevice9**)((char*)dxpDevice + 8);
    }

    PermutedShader(DX_PATCH::IDirect3DDevice9* dxpDevice, const DWORD* data, const size_t dataSize, const bool isPixelShader) : refCount(1), dxpDevice(dxpDevice),
        isPermuted(hlBINAHasV2Header(data))
    {
        memset(&handle, 0, sizeof(handle));

        memory = operator new(dataSize);
        memcpy(memory, data, dataSize);

        if (isPermuted)
        {
            hlBINAV2Fix(memory, dataSize);
            container = (PermutationContainer*)hlBINAV2GetData(memory);

#ifndef PERMUTATION_HANDLER_LAZY_LOADING
            for (size_t i = 0; i < container->shaderCount; i++)
            {
                if (isPixelShader)
                    container->shaders[i].handle.createPixelShader(d3dDevice());
                else
                    container->shaders[i].handle.createVertexShader(d3dDevice());
            }
        }
        else if (isPixelShader)
            handle.createPixelShader(d3dDevice());
        else
            handle.createVertexShader(d3dDevice());
#else
        }
#endif
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
            for (size_t i = 0; i < container->shaderCount; i++)
                container->shaders[i].handle.release();
        }

        else
            handle.release();

        operator delete(memory);
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

        PermutedShaderFunctionHandle* handle;

        if (shader->isPermuted)
        {
            size_t index = 0;

            for (size_t i = 0; i < shader->container->permutationCount; i++)
            {
                if (permutations & (1 << shader->container->permutations[i].registerIndex))
                    index |= 1 << i;
            }

            handle = &shader->container->shaders[shader->container->permutationMap[index]].handle;
        }

        else
            handle = &shader->handle;

        if (isPixelShader)
            shader->d3dDevice()->SetPixelShader(handle->getPixelShader(shader->d3dDevice()));

        else
            shader->d3dDevice()->SetVertexShader(handle->getVertexShader(shader->d3dDevice()));

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

        // TODO: Get rid of this when you fix vertex shader translation
        if (StartRegister == 0)
            return originalDxpDeviceSetVertexShaderConstantB(This, Edx, StartRegister, pConstantData, BoolCount);

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
