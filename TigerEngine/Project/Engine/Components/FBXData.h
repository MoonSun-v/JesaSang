#pragma once
#include <pch.h>
#include "../Object/Component.h"
#include "Datas/Mesh.h"
#include "../Manager/FBXResourceManager.h"

class FBXData : public Component
{
    RTTR_ENABLE(Component)
public:
    void OnInitialize() override;
    std::vector<Mesh>& GetMesh();
    std::shared_ptr<FBXResourceAsset> GetFBXInfo();

    void ChangeData(std::string path);

    nlohmann::json Serialize() override;
	void Deserialize(nlohmann::json data) override;
    
    std::string path = ""; // fbx path data

protected:
    std::vector<Mesh> meshes; // 현재 컴포넌트가 들고 있는 mesh 정보
    std::shared_ptr<FBXResourceAsset> fbxAsset; // 현재 컴포넌트가 들고 있는 mesh 정보
};