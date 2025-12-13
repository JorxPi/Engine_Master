#include "Globals.h"
#include "Application.h"
#include "ModulePipeline.h"
#include "ModuleD3D12.h"
#include "ModuleResources.h"
#include "ReadData.h" 
#include "ModuleCameraEditor.h"
#include "ModuleSampler.h"
#include <SimpleMath.h>
using namespace DirectX::SimpleMath;

ModulePipeline::ModulePipeline() {

}


bool ModulePipeline::init() {
    if (!createQuadVertexBuffer()) return false;
    if (!createRootSignature())    return false;
    if (!createPSO())              return false;

    auto modD3D12 = app->getModule<ModuleD3D12>();
    
    if (!setTextureFromFile(L"Assets/Textures/popcorn.jpg"))
        return false;

    auto device4 = reinterpret_cast<ID3D12Device4*>(modD3D12->getDevice());
    debugDraw = new DebugDrawPass(device4, modD3D12->getCommandQueue());
    return true;
}

void ModulePipeline::preRender() {
    auto modD3D12 = app->getModule<ModuleD3D12>();
    ID3D12GraphicsCommandList* cmd = modD3D12->getCommandList();
    auto modSampler = app->getModule<ModuleSampler>();

    Matrix model = Matrix::Identity;

    auto camera = app->getModule<ModuleCameraEditor>();

    const Matrix& view = camera->getViewMatrix();
    const Matrix& proj = camera->getProjectionMatrix();

    Matrix mvp = (model * view * proj).Transpose();

    RECT rc{}; 
    GetClientRect(modD3D12->getWindowHandle(), &rc);
    float w = float(rc.right - rc.left);
    float h = float(rc.bottom - rc.top);

    //TopLeftX - TopLeftY - ViewportWidth - ViewportHeight - MinDepth - MaxDepth
    D3D12_VIEWPORT viewport{ 0.0, 0.0, float(w), float(h) , 0.0, 1.0 };

    D3D12_RECT sc{ 0, 0, (LONG)w, (LONG)h };
    cmd->RSSetViewports(1, &viewport);
    cmd->RSSetScissorRects(1, &sc);

    auto rtv = modD3D12->getCurrentRTV();
    auto dsv = modD3D12->getDSV();
    cmd->OMSetRenderTargets(1, &rtv, FALSE, &dsv);

    ID3D12DescriptorHeap* heaps[] = { srvHeap.Get(), modSampler->getHeap() };
    cmd->SetDescriptorHeaps(2, heaps);

    cmd->SetGraphicsRootSignature(rootSignature.Get());
    cmd->SetGraphicsRoot32BitConstants(0, sizeof(Matrix) / sizeof(UINT32), &mvp, 0);
    cmd->SetGraphicsRootDescriptorTable(1, textureGpuHandle);
    cmd->SetGraphicsRootDescriptorTable(2, modSampler->getGpuHandle((UINT)selectedSamplerIndex));
    cmd->SetPipelineState(pso.Get());
    cmd->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    cmd->IASetVertexBuffers(0, 1, &vbv);

    cmd->DrawInstanced(6, 1, 0, 0);

    // Grid & Axis

    if (showGrid) dd::xzSquareGrid(-10.0f, 10.0f, 0.0f, 1.0f, dd::colors::LightGray);
    if (showAxis) dd::axisTriad(ddConvert(Matrix::Identity), 0.1f, 1.0f);

    debugDraw->record(cmd, (uint32_t)w, (uint32_t)h, view, proj);
}

struct Vertex {
    Vector3 position;
    Vector2 uv;
};

bool ModulePipeline::createQuadVertexBuffer() {
    auto modRes = app->getModule<ModuleResources>();

    static Vertex vertices[6] =
    {
        { Vector3(-1.0f, -1.0f, 0.0f), Vector2(-0.2f,  1.2f) },
        { Vector3(-1.0f,  1.0f, 0.0f), Vector2(-0.2f, -0.2f) },
        { Vector3(1.0f,  1.0f, 0.0f), Vector2(1.2f, -0.2f) },

        { Vector3(-1.0f, -1.0f, 0.0f), Vector2(-0.2f,  1.2f) },
        { Vector3(1.0f,  1.0f, 0.0f), Vector2(1.2f, -0.2f) },
        { Vector3(1.0f, -1.0f, 0.0f), Vector2(1.2f,  1.2f) },
    };

    const UINT64 sizeBytes = sizeof(vertices);

    vertexBuffer = modRes->createDefaultBuffer(vertices, sizeBytes);

    vbv.BufferLocation = vertexBuffer->GetGPUVirtualAddress();
    vbv.SizeInBytes = static_cast<UINT>(sizeBytes);
    vbv.StrideInBytes = sizeof(Vertex);

    return true;
}

bool ModulePipeline::createRootSignature() {
    auto modD3D12 = app->getModule<ModuleD3D12>();

    CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
    CD3DX12_ROOT_PARAMETER rootParams[3];
    
    CD3DX12_DESCRIPTOR_RANGE tableRange;
    tableRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0); 

    CD3DX12_DESCRIPTOR_RANGE samplerRange;
    samplerRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, 1, 0, 0);

    rootParams[0].InitAsConstants(sizeof(Matrix) / sizeof(UINT32), 0, 0, D3D12_SHADER_VISIBILITY_VERTEX);
    rootParams[1].InitAsDescriptorTable(1, &tableRange, D3D12_SHADER_VISIBILITY_PIXEL);
    rootParams[2].InitAsDescriptorTable(1, &samplerRange, D3D12_SHADER_VISIBILITY_PIXEL);

    rootSignatureDesc.Init(3, rootParams, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

    ComPtr<ID3DBlob> blob;
    if (FAILED(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &blob, nullptr)))
        return false;

    if (FAILED(modD3D12->getDevice()->CreateRootSignature(0, blob->GetBufferPointer(), blob->GetBufferSize(), IID_PPV_ARGS(&rootSignature))))
        return false;

    return true;
}

bool ModulePipeline::createPSO() {
    auto modD3D12 = app->getModule<ModuleD3D12>();
    ID3D12Device* device = modD3D12->getDevice();

    std::vector<uint8_t> dataVS, dataPS;

    dataVS = DX::ReadData(L"ExerciseQuadVer.cso");
    dataPS = DX::ReadData(L"ExerciseQuadPix.cso");

    D3D12_INPUT_ELEMENT_DESC inputLayout[] = {  
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }, 
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0} 
    };

    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
    psoDesc.pRootSignature = rootSignature.Get();
    psoDesc.InputLayout = {inputLayout, sizeof(inputLayout) / sizeof(D3D12_INPUT_ELEMENT_DESC)};

    psoDesc.VS = { dataVS.data(), dataVS.size() };
    psoDesc.PS = { dataPS.data(), dataPS.size() };

    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
    psoDesc.NumRenderTargets = 1;
    psoDesc.SampleDesc = { 1, 0 };
    psoDesc.SampleMask = 0xffffffff;
    psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);

    psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
    psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;

    return SUCCEEDED(device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pso)));
}

bool ModulePipeline::cleanUp() {
    srvHeap.Reset();
    texture.Reset();
    vertexBuffer.Reset();
    rootSignature.Reset();
    pso.Reset();

    delete debugDraw;
    debugDraw = nullptr;
    return true;
}

bool ModulePipeline::setTextureFromFile(const wchar_t* path)
{
    auto modD3D12 = app->getModule<ModuleD3D12>();
    auto modRes = app->getModule<ModuleResources>();
    ID3D12Device* device = modD3D12->getDevice();

    ComPtr<ID3D12Resource> newTex = modRes->createTextureFromFile(path);
    if (!newTex)
        return false;

    D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
    heapDesc.NumDescriptors = 1;
    heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

    ComPtr<ID3D12DescriptorHeap> newSrvHeap;
    if (FAILED(device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&newSrvHeap))))
        return false;

    D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = newSrvHeap->GetCPUDescriptorHandleForHeapStart();
    device->CreateShaderResourceView(newTex.Get(), nullptr, cpuHandle);

    texture = newTex;
    srvHeap = newSrvHeap;
    textureGpuHandle = srvHeap->GetGPUDescriptorHandleForHeapStart();
    currentTexturePath = path;

    return true;
}

void ModulePipeline::setSamplerIndex(int idx)
{
    if (idx < 0) idx = 0;
    if (idx > 3) idx = 3; 
    selectedSamplerIndex = idx;
}