#pragma once

#include <DirectXMath.h>

using Microsoft::WRL::ComPtr;

inline constexpr uint32_t INVALID_DESC = 0xFFFFFFFFu;

class RenderTexture
{
public:
    RenderTexture(
        DXGI_FORMAT colorFmt,
        DXGI_FORMAT depthFmt,
        DirectX::XMFLOAT4 clearColor = { 0,0,0,1 },
        float clearDepth = 1.0f
    );

    ~RenderTexture();

    bool init(int w, int h);
    void resize(int w, int h);
    void cleanUp();

    bool isValid() const;

    void beginRender(ID3D12GraphicsCommandList* cmd);
    void endRender(ID3D12GraphicsCommandList* cmd);

    D3D12_CPU_DESCRIPTOR_HANDLE getRtv() const;
    D3D12_CPU_DESCRIPTOR_HANDLE getDsv() const;
    D3D12_GPU_DESCRIPTOR_HANDLE getSrvGpu() const;

    int getWidth() const { return width; }
    int getHeight() const { return height; }

private:
    void destroy();
    bool createResources(int w, int h);

private:
    DXGI_FORMAT colorFmt;
    DXGI_FORMAT depthFmt;
    DirectX::XMFLOAT4 clear;
    float clearDepth = 1.0f;

    int width = 0;
    int height = 0;

    ComPtr<ID3D12Resource> color;
    ComPtr<ID3D12Resource> depth;

    uint32_t rtvIdx = INVALID_DESC;
    uint32_t dsvIdx = INVALID_DESC;
    uint32_t srvIdx = INVALID_DESC;
};
