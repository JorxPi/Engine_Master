#pragma once
#include "Module.h"
#include "DebugDrawPass.h"
#include "Model.h"
#include "PhongData.h"
#include "RenderTexture.h"
#include "LightSystem.h"

class ModulePipeline : public Module
{
public:
    ModulePipeline();
    bool init() override;
    void preRender() override;
    bool cleanUp() override;

    void initPBRPhongSettings();

    void setSamplerIndex(int i);

    void setSceneSize(int w, int h);

    void setShowGrid(bool show) { showGrid = show; }
    void setShowAxis(bool show) { showAxis = show; }

    const wchar_t* getTexturePath() const { return currentTexturePath.c_str(); }

    bool getShowGrid() const { return showGrid; }
    bool getShowAxis() const { return showAxis; }

    const Model& getModel() const { return duckModel; }
    Model& getModel() { return duckModel; }

    PhongSettings& editPhong() { return phong; }
    const PhongSettings& getPhong() const { return phong; }

    RenderTexture* getSceneRT() const { return sceneRT.get(); }

    //Lights

    LightSystem& editLightSystem() { return lightSystem; }

    OwnerId getDirectionalOwner() const { return directionalOwner; }
    OwnerId getPointOwner() const { return pointOwner; }
    OwnerId getSpotOwner() const { return spotOwner; }

    LightId getDirectionalLight() const { return directionalLight; }
    LightId getPointLight() const { return pointLight; }
    LightId getSpotLight() const { return spotLight; }

private:
    bool createRootSignature();
    bool createPSO();
    void createProvisionalLights();

    Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> pso;

    std::wstring currentTexturePath;

    Model duckModel;

    uint32_t nullSrvIndex = 0;

    PhongSettings phong;

    std::unique_ptr<RenderTexture> sceneRT;

    // Grid
    DebugDrawPass* debugDraw = nullptr;

    bool showGrid = true;
    bool showAxis = true;

    //Light

    LightSystem lightSystem;
    OwnerId directionalOwner = 0;
    OwnerId pointOwner = 0;
    OwnerId spotOwner = 0;

    LightId directionalLight = 0;
    LightId pointLight = 0;
    LightId spotLight = 0;

};
