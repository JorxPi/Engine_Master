#include "Globals.h"
#include "Application.h"
#include "ModulePipeline.h"
#include "ModuleD3D12.h"
#include "ModuleResources.h"
#include "ReadData.h" 
#include "ModuleCameraEditor.h"
#include "ModuleSampler.h"
#include "ModuleShaderDescriptors.h"
#include "ModuleRingBuffer.h"
#include "DebugDrawPass.h"
#include "LightDebugDraw.h"
#include <SimpleMath.h>

static inline const float* asFloat3(const Vector3& v)
{
    return &v.x;
}

ModulePipeline::ModulePipeline() {

}


bool ModulePipeline::init() {
    if (!createRootSignature())    return false;
    if (!createPSO())              return false;

    auto modD3D12 = app->getModule<ModuleD3D12>();
    auto modDesc = app->getModule<ModuleShaderDescriptors>();
    nullSrvIndex = modDesc->createNullTexture2DSRV();

    planeMesh.createPlane(10.0f);
    
    duckModel.loadModel("Assets/Models/Duck/Duck.gltf");
    duckModel.setModelMatrix(Matrix::CreateScale(0.01f));
    LOG("Duck meshes: %zu  materials: %zu",
        duckModel.getMeshes().size(),
        duckModel.getMaterials().size());

    initPBRPhongSettings();

    createProvisionalLights();

    sceneRT = std::make_unique<RenderTexture>(DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_FORMAT_D32_FLOAT, DirectX::XMFLOAT4(0.188f, 0.208f, 0.259f, 1.0f), 1.0f);

    auto device4 = reinterpret_cast<ID3D12Device4*>(modD3D12->getDevice());
    debugDraw = new DebugDrawPass(device4, modD3D12->getCommandQueue());
    return true;
}

void ModulePipeline::preRender()
{
    auto modD3D12 = app->getModule<ModuleD3D12>();
    ID3D12GraphicsCommandList* cmd = modD3D12->getCommandList();

    auto modSampler = app->getModule<ModuleSampler>();
    auto modDesc = app->getModule<ModuleShaderDescriptors>();
    auto camera = app->getModule<ModuleCameraEditor>();
    auto ring = app->getModule<ModuleRingBuffer>();

    if (!sceneRT || !sceneRT->isValid())
        return;

    sceneRT->beginRender(cmd);

    const uint32_t w = (uint32_t)sceneRT->getWidth();
    const uint32_t h = (uint32_t)sceneRT->getHeight();

    const Matrix& view = camera->getViewMatrix();
    const Matrix& proj = camera->getProjectionMatrix();

    ID3D12DescriptorHeap* heaps[] = { modDesc->getHeap(), modSampler->getHeap() };
    cmd->SetDescriptorHeaps(2, heaps);

    cmd->SetGraphicsRootSignature(rootSignature.Get());
    cmd->SetPipelineState(pso.Get());
    cmd->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    PerFrame perFrame{};
    perFrame.viewPos = camera->readCamera().position;

    auto perFrameAllocation = ring->allocBuffer((uint32_t)sizeof(PerFrame));
    memcpy(perFrameAllocation.cpu, &perFrame, sizeof(PerFrame));
    cmd->SetGraphicsRootConstantBufferView(1, perFrameAllocation.gpu);

    GPULightsConstantBuffer gpuLights = lightSystem.packForGPUConstantBuffer();

    auto lightsAllocation = ring->allocBuffer((uint32_t)sizeof(GPULightsConstantBuffer));
    memcpy(lightsAllocation.cpu, &gpuLights, sizeof(GPULightsConstantBuffer));
    cmd->SetGraphicsRootConstantBufferView(3, lightsAllocation.gpu);

    cmd->SetGraphicsRootDescriptorTable(5, modSampler->getGpuHandle((UINT)phong.samplerIndex));

    // DRAW PLANE
    {
        Matrix planeModel = Matrix::CreateTranslation(0.0f, -0.01f, 0.0f);

        Matrix planeMvp = (planeModel * view * proj).Transpose();
        cmd->SetGraphicsRoot32BitConstants(0, sizeof(Matrix) / sizeof(UINT32), &planeMvp, 0);

        Matrix planeModelT = planeModel.Transpose();

        Matrix planeNormalMat = planeModel;
        planeNormalMat.Invert();
        planeNormalMat = planeNormalMat.Transpose();

        PerInstance perInst{};
        perInst.modelMat = planeModelT;
        perInst.normalMat = planeNormalMat;

        auto mat = phong.overrideMat;
        mat.hasDiffuseTex = FALSE; 
        mat.diffuseColour = XMFLOAT3(0.7f, 0.7f, 0.7f); 
        perInst.material = mat;

        auto piAlloc = ring->allocBuffer((uint32_t)sizeof(PerInstance));
        memcpy(piAlloc.cpu, &perInst, sizeof(PerInstance));

        cmd->SetGraphicsRootConstantBufferView(2, piAlloc.gpu);

        cmd->SetGraphicsRootDescriptorTable(4, modDesc->getGpuHandle(nullSrvIndex));

        cmd->IASetVertexBuffers(0, 1, &planeMesh.getVBV());
        if (planeMesh.hasIndices()) cmd->IASetIndexBuffer(&planeMesh.getIBV());

        if (planeMesh.hasIndices()) cmd->DrawIndexedInstanced(planeMesh.getNumIndices(), 1, 0, 0, 0);
        else                        cmd->DrawInstanced(planeMesh.getNumVertices(), 1, 0, 0);
    }

    // DRAW DUCK
    {
        const Matrix model = duckModel.getModelMatrix();

        Matrix mvp = (model * view * proj).Transpose();
        cmd->SetGraphicsRoot32BitConstants(0, sizeof(Matrix) / sizeof(UINT32), &mvp, 0);

        const auto& meshes = duckModel.getMeshes();
        const auto& mats = duckModel.getMaterials();

        const Matrix modelT = model.Transpose();

        Matrix normalMat = model;
        normalMat.Invert();
        normalMat = normalMat.Transpose();

        for (const Mesh& mesh : meshes)
        {
            cmd->IASetVertexBuffers(0, 1, &mesh.getVBV());
            if (mesh.hasIndices()) cmd->IASetIndexBuffer(&mesh.getIBV());

            const int matIdx = mesh.getMaterialIndex();

            PerInstance perInst{};
            perInst.modelMat = modelT;
            perInst.normalMat = normalMat;

            PBRPhongMaterialData matData{};
            uint32_t srvIndex = nullSrvIndex;

            if (matIdx >= 0 && matIdx < (int)mats.size())
            {
                const auto& srcMat = mats[matIdx];

                if (phong.useOverride)
                {
                    matData = phong.overrideMat;
                    srvIndex = matData.hasDiffuseTex ? srcMat.getColourSrvIndex() : nullSrvIndex;
                }
                else
                {
                    matData = srcMat.getPBRPhong();
                    srvIndex = srcMat.getColourSrvIndex();
                }
            }
            else
            {
                matData = phong.overrideMat;
                srvIndex = nullSrvIndex;
            }

            perInst.material = matData;

            auto piAlloc = ring->allocBuffer((uint32_t)sizeof(PerInstance));
            memcpy(piAlloc.cpu, &perInst, sizeof(PerInstance));

            cmd->SetGraphicsRootConstantBufferView(2, piAlloc.gpu);
            cmd->SetGraphicsRootDescriptorTable(4, modDesc->getGpuHandle(srvIndex));

            if (mesh.hasIndices()) cmd->DrawIndexedInstanced(mesh.getNumIndices(), 1, 0, 0, 0);
            else cmd->DrawInstanced(mesh.getNumVertices(), 1, 0, 0);
        }
    }

    // DEBUG DRAW
    if (showGrid) dd::xzSquareGrid(-10.0f, 10.0f, 0.0f, 1.0f, dd::colors::LightGray);
    if (showAxis) dd::axisTriad(ddConvert(Matrix::Identity), 0.1f, 1.0f);


    if (showDirectionalLightDebugDraw || showPointLightDebugDraw || showSpotLightDebugDraw)
    {
        const LightInstance* inst = lightSystem.getLight(debugLightId);
        if (inst)
        {
            bool draw = false;
            if (inst->lightComponent.type == LightType::DIRECTIONAL) {
                draw = showDirectionalLightDebugDraw;
            }
            if (inst->lightComponent.type == LightType::POINT) {
                draw = showPointLightDebugDraw;
            }
            if (inst->lightComponent.type == LightType::SPOT) {
                draw = showSpotLightDebugDraw;
            }

            if (draw)
                LightDebugDraw::drawLight(lightSystem, debugLightId);
        }
    }
    
    debugDraw->record(cmd, w, h, view, proj);

    sceneRT->endRender(cmd);
}


struct Vertex {
    Vector3 position;
    Vector2 uv;
};

bool ModulePipeline::createRootSignature() {
    auto modD3D12 = app->getModule<ModuleD3D12>();

    CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
    CD3DX12_ROOT_PARAMETER rootParams[6];
    
    CD3DX12_DESCRIPTOR_RANGE tableRange;
    tableRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0); 

    CD3DX12_DESCRIPTOR_RANGE samplerRange;
    samplerRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, 1, 0, 0);

    rootParams[0].InitAsConstants(sizeof(Matrix) / sizeof(UINT32), 0, 0, D3D12_SHADER_VISIBILITY_VERTEX); // b0
    rootParams[1].InitAsConstantBufferView(1, 0, D3D12_SHADER_VISIBILITY_ALL); // b1 
    rootParams[2].InitAsConstantBufferView(2, 0, D3D12_SHADER_VISIBILITY_ALL); // b2 
    rootParams[3].InitAsConstantBufferView(3, 0, D3D12_SHADER_VISIBILITY_PIXEL); // b3 (LightsCB)
    rootParams[4].InitAsDescriptorTable(1, &tableRange, D3D12_SHADER_VISIBILITY_PIXEL); // t0
    rootParams[5].InitAsDescriptorTable(1, &samplerRange, D3D12_SHADER_VISIBILITY_PIXEL); // s0

    rootSignatureDesc.Init(6, rootParams, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

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

    dataVS = DX::ReadData(L"PhongVS.cso");
    dataPS = DX::ReadData(L"LightsPS.cso");

    D3D12_INPUT_ELEMENT_DESC inputLayout[] = {  
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }, 
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
        { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
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
    psoDesc.RasterizerState.FrontCounterClockwise = TRUE;

    psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);

    psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
    psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;

    return SUCCEEDED(device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pso)));
}

bool ModulePipeline::cleanUp() {
    rootSignature.Reset();
    pso.Reset();

    delete debugDraw;
    debugDraw = nullptr;
    return true;
}

void ModulePipeline::initPBRPhongSettings()
{
    phong.lightDir.Normalize();

    phong.overrideMat.diffuseColour = XMFLOAT3(1, 1, 1);
    phong.overrideMat.hasDiffuseTex = TRUE;

    phong.overrideMat.specularColour = XMFLOAT3(0.04f, 0.04f, 0.04f);
    phong.overrideMat.shininess = 64.0f;
}

void ModulePipeline::setSamplerIndex(int idx)
{
    if (idx < 0) idx = 0;
    if (idx > 3) idx = 3; 
    phong.samplerIndex = idx;
}

void ModulePipeline::setSceneSize(int w, int h)
{
    w = (w < 1) ? 1 : w;
    h = (h < 1) ? 1 : h;

    if (!sceneRT) return;

    if (sceneRT->isValid() &&
        w == sceneRT->getWidth() &&
        h == sceneRT->getHeight())
        return;

    sceneRT->resize(w, h);

    if (auto cam = app->getModule<ModuleCameraEditor>())
        cam->requestResize((uint32_t)w, (uint32_t)h);
}

void ModulePipeline::createProvisionalLights()
{
    debugLightOwner = lightSystem.createOwner({ Vector3(0, 2, 0), Vector3::Forward });

    LightCommon common{};
    common.enabled = true;
    common.color = Vector3(1, 0.9f, 0.7f);
    common.intensity = 20.0f;

    PointLightParameters pointParams{};
    pointParams.radius = 8.0f;

    debugLightId = lightSystem.createPointLight(debugLightOwner, common, pointParams);

    lightSystem.setAmbient(Vector3(0.1f, 0.1f, 0.1f), 1.0f);
}