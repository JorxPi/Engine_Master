#pragma once
#include "Module.h"
#include "DebugDrawPass.h"
#include "Model.h"


class ModulePipeline : public Module
{
public:
    ModulePipeline();
    bool init() override;
    void preRender() override;
    bool cleanUp() override;

    //bool setTextureFromFile(const wchar_t* path);

    void setSamplerIndex(int i);

    void setShowGrid(bool show) { showGrid = show; }
    void setShowAxis(bool show) { showAxis = show; }

    const wchar_t* getTexturePath() const { return currentTexturePath.c_str(); }

    bool getShowGrid() const { return showGrid; }
    bool getShowAxis() const { return showAxis; }

    const Model& getModel() const { return duckModel; }
    Model& getModel() { return duckModel; }

private:
    //bool createQuadVertexBuffer();
    bool createRootSignature();
    bool createPSO();

    Microsoft::WRL::ComPtr<ID3D12Resource> vertexBuffer;
    D3D12_VERTEX_BUFFER_VIEW vbv{};
    Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> pso;

    std::wstring currentTexturePath;

    Model duckModel;

    ComPtr<ID3D12Resource> defaultMaterialBuffer;
    uint32_t nullSrvIndex = 0;

    int selectedSamplerIndex = 0;


    // Grid
    DebugDrawPass* debugDraw = nullptr;

    bool showGrid = true;
    bool showAxis = true;

};
