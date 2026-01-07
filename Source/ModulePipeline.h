#pragma once
#include "Module.h"
#include "DebugDrawPass.h"
#include "Model.h"
#include "PhongData.h"


class ModulePipeline : public Module
{
public:
    ModulePipeline();
    bool init() override;
    void preRender() override;
    bool cleanUp() override;

    void initPBRPhongSettings();

    void setSamplerIndex(int i);

    void setShowGrid(bool show) { showGrid = show; }
    void setShowAxis(bool show) { showAxis = show; }

    const wchar_t* getTexturePath() const { return currentTexturePath.c_str(); }

    bool getShowGrid() const { return showGrid; }
    bool getShowAxis() const { return showAxis; }

    const Model& getModel() const { return duckModel; }
    Model& getModel() { return duckModel; }

    PhongSettings& editPhong() { return phong; }
    const PhongSettings& getPhong() const { return phong; }

private:
    bool createRootSignature();
    bool createPSO();

    Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> pso;

    std::wstring currentTexturePath;

    Model duckModel;

    uint32_t nullSrvIndex = 0;

    PhongSettings phong;


    // Grid
    DebugDrawPass* debugDraw = nullptr;

    bool showGrid = true;
    bool showAxis = true;

};
