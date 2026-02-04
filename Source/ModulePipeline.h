#pragma once
#include "Module.h"
#include "DebugDrawPass.h"
#include "Model.h"
#include "PhongData.h"
#include "RenderTexture.h"
#include "LightSystem.h"

class GameObject;
class LightComponent;

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

    bool& editShowDirectionalLightDebugDraw() { return showDirectionalLightDebugDraw; }
    bool& editShowPointLightDebugDraw() { return showPointLightDebugDraw; }
    bool& editShowSpotLightDebugDraw() { return showSpotLightDebugDraw; }

    GameObject* getSingleLightGO() { return m_lightGO; }

private:
    bool createRootSignature();
    bool createPSO();
    void createProvisionalLights();

    Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> pso;

    std::wstring currentTexturePath;

    Model duckModel;
    Mesh planeMesh;

    uint32_t nullSrvIndex = 0;

    PhongSettings phong;

    std::unique_ptr<RenderTexture> sceneRT;

    // Grid
    DebugDrawPass* debugDraw = nullptr;

    bool showGrid = true;
    bool showAxis = true;

    //Light
    bool showDirectionalLightDebugDraw = true;
    bool showPointLightDebugDraw = true;
    bool showSpotLightDebugDraw = true;

    LightSystem lightSystem;

    //tempObjects is the temporary scene
    std::vector<GameObject*> m_tempObjects;
    GameObject* m_lightGO = nullptr;
    LightComponent* m_lightComp = nullptr;

};
