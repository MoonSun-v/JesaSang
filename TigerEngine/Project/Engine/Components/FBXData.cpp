#include "FBXData.h"
#include "Helper.h"
#include "../Manager/ComponentFactory.h"
#include "../Components/FBXRenderer.h"

RTTR_REGISTRATION
{
    rttr::registration::class_<FBXData>("FBXData")
        .constructor<>()
            (rttr::policy::ctor::as_std_shared_ptr)
        .property("DataPath", &FBXData::path)
        .property("IsStatic", &FBXData::isStatic);
}

void FBXData::OnInitialize()
{
    // 임시
    isStatic = false;
    path = "..\\Assets\\Resource\\sphere.fbx";
    fbxAsset = FBXResourceManager::Instance().LoadStaticFBXByPath("..\\Assets\\Resource\\sphere.fbx");
    meshes = fbxAsset->meshes; 

    owner->SetAABB(fbxAsset->boxMin, fbxAsset->boxMax, fbxAsset->boxCenter);

    auto renderer = owner->GetComponent<FBXRenderer>();
    if(renderer != nullptr) renderer->OnInitialize();
}

std::vector<Mesh>& FBXData::GetMesh()
{
    return meshes;
}

std::shared_ptr<FBXResourceAsset> FBXData::GetFBXInfo()
{
    return fbxAsset;
}

void FBXData::ChangeData(std::string path)
{
    isStatic = false;
    auto getData = FBXResourceManager::Instance().LoadFBXByPath(path);
    fbxAsset.reset();
    fbxAsset = getData;
    meshes = fbxAsset->meshes; 
    this->path = path;
    owner->SetAABB(fbxAsset->boxMin, fbxAsset->boxMax, fbxAsset->boxCenter);

    auto renderer = owner->GetComponent<FBXRenderer>(); 
    if(renderer != nullptr) renderer->OnInitialize();
}

void FBXData::ChangeStaticData(std::string path)
{
    isStatic = true;
    auto getData = FBXResourceManager::Instance().LoadStaticFBXByPath(path);
    fbxAsset.reset();
    fbxAsset = getData;
    meshes = fbxAsset->meshes;
    this->path = path;
    owner->SetAABB(fbxAsset->boxMin, fbxAsset->boxMax, fbxAsset->boxCenter);

    auto renderer = owner->GetComponent<FBXRenderer>();
    if (renderer != nullptr) renderer->OnInitialize();
}

nlohmann::json FBXData::Serialize()
{
    nlohmann::json datas;

    rttr::type t = rttr::type::get(*this);
    datas["type"] = t.get_name().to_string();
    datas["properties"] = nlohmann::json::object();

    for(auto& prop : t.get_properties())
    {
        std::string propName = prop.get_name().to_string();
        rttr::variant value = prop.get_value(*this);
        if(value.is_type<std::string>())
        {
            auto v = value.get_value<std::string>();
            datas["properties"][propName] = v;
        }
        else if (value.is_type<bool>())
        {
            auto v = value.get_value<bool>();
            datas["properties"][propName] = v;
        }
	}

    return datas;
}

void FBXData::Deserialize(nlohmann::json data)
{
    // data : data["objects"]["properties"]["components"]["현재 컴포넌트"]

    auto propData = data["properties"];

    if (propData.contains("IsStatic"))
        isStatic = propData["IsStatic"].get<bool>(); // 강제로 먼저 찾기 -> NOTE: 만약 로드할 때 안 Rigid로만 잡히면 확인

    rttr::type t = rttr::type::get(*this);
    for(auto& prop : t.get_properties())
    {
        std::string propName = prop.get_name().to_string();
        rttr::variant value = prop.get_value(*this);
	    if(value.is_type<std::string>() && propName == "DataPath")
        {
            std::string str = propData["DataPath"];
            prop.set_value(*this, str);

            if(isStatic)
                fbxAsset = FBXResourceManager::Instance().LoadStaticFBXByPath(str);
            else
                fbxAsset = FBXResourceManager::Instance().LoadFBXByPath(str);
            meshes = fbxAsset->meshes; 
            owner->SetAABB(fbxAsset->boxMin, fbxAsset->boxMax, fbxAsset->boxCenter);
        }
	}
}
