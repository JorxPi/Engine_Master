#include "Globals.h"
#include "RenderTexture.h"
#include "Application.h"
#include "ModuleD3D12.h"
#include "ModuleShaderDescriptors.h"
#include "ModuleRTVDescriptors.h"
#include "ModuleDSVDescriptors.h"
#include "ModuleResources.h"

#include "d3dx12.h"

RenderTexture::RenderTexture(DXGI_FORMAT cFmt, DXGI_FORMAT dFmt, DirectX::XMFLOAT4 clearColor, float depthClear)
    : colorFmt(cFmt), depthFmt(dFmt), clear(clearColor), clearDepth(depthClear) {
}

RenderTexture::~RenderTexture()
{
    cleanUp();
}

bool RenderTexture::init(int w, int h)
{
    if (w <= 0 || h <= 0) return false;
    destroy();
    return createResources(w, h);
}

void RenderTexture::resize(int w, int h)
{
    if (w <= 0 || h <= 0) return;
    if (w == width && h == height && isValid()) return;

    destroy();
    createResources(w, h);
}

void RenderTexture::cleanUp()
{
    destroy();
}

bool RenderTexture::isValid() const
{
    return color != nullptr
        && depth != nullptr
        && width > 0
        && height > 0
        && rtvIdx != INVALID_DESC
        && dsvIdx != INVALID_DESC
        && srvIdx != INVALID_DESC;
}

void RenderTexture::beginRender(ID3D12GraphicsCommandList* cmd)
{
    if (!cmd || !isValid()) return;

    // SRV to RTV
    {
        auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(color.Get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);
        cmd->ResourceBarrier(1, &barrier);
    }

    D3D12_VIEWPORT vp{};
    vp.TopLeftX = 0.0f;
    vp.TopLeftY = 0.0f;
    vp.Width = (float)width;
    vp.Height = (float)height;
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;

    D3D12_RECT sc{};
    sc.left = 0;
    sc.top = 0;
    sc.right = (LONG)width;
    sc.bottom = (LONG)height;

    cmd->RSSetViewports(1, &vp);
    cmd->RSSetScissorRects(1, &sc);

    D3D12_CPU_DESCRIPTOR_HANDLE rtv = getRtv();
    D3D12_CPU_DESCRIPTOR_HANDLE dsv = getDsv();
    cmd->OMSetRenderTargets(1, &rtv, FALSE, &dsv);

    float cc[4] = { clear.x, clear.y, clear.z, clear.w };
    cmd->ClearRenderTargetView(rtv, cc, 0, nullptr);
    cmd->ClearDepthStencilView(dsv, D3D12_CLEAR_FLAG_DEPTH, clearDepth, 0, 0, nullptr);
}

void RenderTexture::endRender(ID3D12GraphicsCommandList* cmd)
{
    if (!cmd || !isValid()) return;

    // RTV to SRV
    auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(color.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    cmd->ResourceBarrier(1, &barrier);
}

D3D12_CPU_DESCRIPTOR_HANDLE RenderTexture::getRtv() const
{
    auto* rtvDesc = app->getModule<ModuleRTVDescriptors>();
    return rtvDesc->getCpuHandle(rtvIdx);
}

D3D12_CPU_DESCRIPTOR_HANDLE RenderTexture::getDsv() const
{
    auto* dsvDesc = app->getModule<ModuleDSVDescriptors>();
    return dsvDesc->getCpuHandle(dsvIdx);
}

D3D12_GPU_DESCRIPTOR_HANDLE RenderTexture::getSrvGpu() const
{
    auto* shaderDesc = app->getModule<ModuleShaderDescriptors>();
    return shaderDesc->getGpuHandle(srvIdx);
}

bool RenderTexture::createResources(int w, int h)
{
    width = w;
    height = h;

    auto* res = app->getModule<ModuleResources>();
    auto* shaderDesc = app->getModule<ModuleShaderDescriptors>();
    auto* rtvDesc = app->getModule<ModuleRTVDescriptors>();
    auto* dsvDesc = app->getModule<ModuleDSVDescriptors>();
    auto* d3d12 = app->getModule<ModuleD3D12>();

    if (!res || !shaderDesc || !rtvDesc || !dsvDesc || !d3d12) return false;

    // --- Color resource ---
    float cc[4] = { clear.x, clear.y, clear.z, clear.w };
    color = res->createRenderTarget(colorFmt, (UINT)width, (UINT)height, cc);
    if (!color) { destroy(); return false; }

    rtvIdx = rtvDesc->createRTV(color.Get(), nullptr);
    srvIdx = shaderDesc->createTexture2DSRV(color.Get());
    if (rtvIdx == INVALID_DESC || srvIdx == INVALID_DESC) { destroy(); return false; }

    {
        ID3D12GraphicsCommandList* cmd = d3d12->getCommandList();
        auto* alloc = d3d12->getCommandAllocator();
        auto* queue = d3d12->getCommandQueue();

        alloc->Reset();
        cmd->Reset(alloc, nullptr);

        auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
            color.Get(),
            D3D12_RESOURCE_STATE_COMMON,
            D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
        );
        cmd->ResourceBarrier(1, &barrier);

        cmd->Close();
        ID3D12CommandList* lists[] = { cmd };
        queue->ExecuteCommandLists(1, lists);
        d3d12->flush();
    }

    // --- Depth resource ---
    depth = res->createDepthStencil(depthFmt, (UINT)width, (UINT)height, clearDepth, 0);
    if (!depth) { destroy(); return false; }

    dsvIdx = dsvDesc->createDSV(depth.Get(), nullptr);
    if (dsvIdx == INVALID_DESC) { destroy(); return false; }

    return true;
}


void RenderTexture::destroy()
{
    depth.Reset();
    color.Reset();

    if (srvIdx != INVALID_DESC)
    {
        if (auto* shaderDesc = app->getModule<ModuleShaderDescriptors>())
            shaderDesc->freeIndex(srvIdx);
    }
    if (rtvIdx != INVALID_DESC)
    {
        if (auto* rtvDesc = app->getModule<ModuleRTVDescriptors>())
            rtvDesc->freeIndex(rtvIdx);
    }
    if (dsvIdx != INVALID_DESC)
    {
        if (auto* dsvDesc = app->getModule<ModuleDSVDescriptors>())
            dsvDesc->freeIndex(dsvIdx);
    }

    srvIdx = rtvIdx = dsvIdx = INVALID_DESC;
    width = height = 0;
}

