#include "FBXData.h"
#include "Helper.h"
#include "../Manager/ComponentFactory.h"
#include "../Components/FBXRenderer.h"
#include "../Util/JsonHelper.h"

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
    //isStatic = false;
    //path = "..\\Assets\\Resource\\sphere.fbx";
    //fbxAsset = FBXResourceManager::Instance().LoadStaticFBXByPath("..\\Assets\\Resource\\sphere.fbx");
    //meshes = fbxAsset->meshes; 
    //owner->SetAABB(fbxAsset->boxMin, fbxAsset->boxMax, fbxAsset->boxCenter);

    auto renderer = owner->GetComponent<FBXRenderer>();
    if(renderer != nullptr) renderer->OnInitialize();
}

void FBXData::OnStart()
{
    cout << "Component_OnStart() : FBXDatat OnStart() check...\n"; // Component OnStart 체크
}

void FBXData::OnDestory()
{
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
    return JsonHelper::MakeSaveData(this);
}

void FBXData::Deserialize(nlohmann::json data)
{
    // data : data["objects"]["properties"]["components"]["현재 컴포넌트"]
    JsonHelper::SetDataFromJson(this, data); // 1. Json으로 불러온 데이터를 설정한다.

    // 2. 설정한 데이터를 가지고 컴포넌트 설정.
    if (path != "")
    {
        if (isStatic)
            fbxAsset = FBXResourceManager::Instance().LoadStaticFBXByPath(path);
        else
            fbxAsset = FBXResourceManager::Instance().LoadFBXByPath(path);

        meshes = fbxAsset->meshes;
        owner->SetAABB(fbxAsset->boxMin, fbxAsset->boxMax, fbxAsset->boxCenter);
    }
}
