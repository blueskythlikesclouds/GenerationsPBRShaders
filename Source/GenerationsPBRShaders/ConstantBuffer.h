#pragma once

template<typename T, size_t StartSlot, bool PS>
class ConstantBuffer : public T
{
    ComPtr<ID3D11Buffer> buffer;

public:
    void update(void* dxpDevice)
    {
        ID3D11DeviceContext* deviceContext = GenerationsD3D11::GetDeviceContext(dxpDevice);

        if (!buffer)
        {
            ID3D11Device* device = GenerationsD3D11::GetDevice(dxpDevice);

            D3D11_BUFFER_DESC bufferDesc{};
            bufferDesc.ByteWidth = (sizeof(T) + 16) & ~0xF;
            bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
            bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
            bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

            D3D11_SUBRESOURCE_DATA initialData{};
            initialData.pSysMem = this;

            device->CreateBuffer(&bufferDesc, &initialData, buffer.GetAddressOf());
        }
        else
        {
            const auto lock = GenerationsD3D11::LockGuard(dxpDevice);

            D3D11_MAPPED_SUBRESOURCE mappedSubResource;
            deviceContext->Map(buffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedSubResource);
            memcpy(mappedSubResource.pData, this, sizeof(T));
            deviceContext->Unmap(buffer.Get(), 0);
        }

        if (PS)
            deviceContext->PSSetConstantBuffers(StartSlot, 1, buffer.GetAddressOf());
        else
            deviceContext->VSSetConstantBuffers(StartSlot, 1, buffer.GetAddressOf());
    }
};