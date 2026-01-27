#include "FBXRenderer.h"
#include "../Manager/ComponentFactory.h"
#include "../Scene/Scene.h"
#include "../Components/FBXData.h"
#include "../Manager/ShaderManager.h"
#include "../Object/GameObject.h"
#include "../Util/JsonHelper.h"
#include "AnimationController.h"
#include "../EngineSystem/PlayModeSystem.h"

RTTR_REGISTRATION
{
    rttr::registration::class_<FBXRenderer>("FBXRenderer")
        .constructor<>()
            (rttr::policy::ctor::as_std_shared_ptr)

        .property("Diffuse", 	    &FBXRenderer::GetDiffuse,       &FBXRenderer::SetDiffuse)
        .property("Alpha", 	        &FBXRenderer::GetAlpha,         &FBXRenderer::SetAlpha)
        .property("Emissive", 	    &FBXRenderer::GetEmissive,		&FBXRenderer::SetEmissive)
        .property("Roughness", 	    &FBXRenderer::GetRoughness,		&FBXRenderer::SetRoughness)
        .property("Metalic", 		&FBXRenderer::GetMatalic,		&FBXRenderer::SetMatalic)

        .property("UseDiffuseOverride", 	&FBXRenderer::GetUseDiffuseOverride,	&FBXRenderer::SetUseDiffuseOverride)
        .property("UseEmissiveOverride", 	&FBXRenderer::GetUseEmissiveOverride,	&FBXRenderer::SetUseEmissiveOverride)
        .property("UseMetallicOverride", 	&FBXRenderer::GetUseRoughnessOverride,	&FBXRenderer::SetUseRoughnessOverride)
        .property("UseRoughnessOverride", 	&FBXRenderer::GetUseMatalicOverride,	&FBXRenderer::SetUseMatalicOverride)

        .property("DiffuseOverride", 	    &FBXRenderer::GetDiffuseOverride,		&FBXRenderer::SetDiffuseOverride)
        .property("EmissiveOverride", 	    &FBXRenderer::GetEmissiveOverride,		&FBXRenderer::SetEmissiveOverride)
        .property("MetallicOverride", 		&FBXRenderer::GetMetallicOverride,		&FBXRenderer::SetMetallicOverride)
        .property("RoughnessOverride",  	&FBXRenderer::GetRoughnessOverride,		&FBXRenderer::SetRoughnessOverride);


		rttr::registration::class_<DirectX::SimpleMath::Color>("Color")
		.constructor<>()
		.constructor<float, float, float>()
		.property("r", &Color::x)
		.property("g", &Color::y)
		.property("b", &Color::z)
		.property("a", &Color::w);
}

void FBXRenderer::OnInitialize()
{
    fbxData = owner->GetComponent<FBXData>();
	if(fbxData != nullptr) CreateBoneInfo(); 
}

void FBXRenderer::OnStart()
{
    cout << "RenderComponent_OnStart() : FBXRenderer OnStart() check...\n"; // RenderComponent OnStart 체크
}

void FBXRenderer::OnUpdate(float delta)
{
    auto anim = owner->GetComponent<AnimationController>();

    if (anim && PlayModeSystem::Instance().IsPlaying()) // Transpose O 
    {
        const auto& pose = anim->AnimatorInstance.GetFinalPose();

        for (int i = 0; i < pose.size(); i++)
        {
            bonePoses.bonePose[i] = pose[i].Transpose();
        }
    }
    else  // Transpose X , bind pose
    {
        auto modelAsset = fbxData->GetFBXInfo();
        for (int i = 0; i < modelAsset->skeletalInfo.m_bones.size(); i++)
        {
            bonePoses.bonePose[i] = modelAsset->skeletalInfo.m_bones[i].globalBind;
        }
    }
}

void FBXRenderer::OnDestory()
{
}

void FBXRenderer::OnRender(RenderQueue& queue)
{
    if (fbxData == nullptr) return;

    ModelType modelType = fbxData->GetFBXInfo()->type;

    auto& meshData = fbxData->GetMesh();
    auto world = owner->GetTransform()->GetWorldTransform();
    auto fbxRoot = fbxData->GetFBXInfo()->rootTransform;

    // Render Item Push
    for (int i = 0; i < meshData.size(); i++)
    {
        auto& mesh = meshData[i];

        RenderItem item{};
        item.modelType = modelType;
        item.mesh = &mesh;
        item.material = mesh.GetMaterial();
        item.world = world;
        
        switch (modelType)
        {
        case ModelType::Skeletal:
            item.boneCount = fbxData->GetFBXInfo()->skeletalInfo.m_bones.size();
            item.refBoneIndex = mesh.refBoneIndex;
            item.poses = &bonePoses;
            item.offsets = &fbxData->GetFBXInfo()->m_BoneOffsets;
            break;
        case ModelType::Rigid:
            item.model = fbxData->GetFBXInfo()->meshes_modelMat[i];
            break;
        case ModelType::Static:
            item.world = world;
            break;
        }

        queue.AddRenderItem(item);
    }
}

void FBXRenderer::CreateBoneInfo()
{
    auto modelAsset = fbxData->GetFBXInfo();
    int size = modelAsset->skeletalInfo.m_bones.size();

    // bind pose로 초기화
    for (int i = 0; i < size; i++)
    {
        bonePoses.bonePose[i] = modelAsset->skeletalInfo.m_bones[i].globalBind.Transpose();
    }
}

nlohmann::json FBXRenderer::Serialize()
{
    return JsonHelper::MakeSaveData(this);
}

void FBXRenderer::Deserialize(nlohmann::json data)
{
	// data : data["objects"]["properties"]["components"]["현재 컴포넌트"]

    JsonHelper::SetDataFromJson(this, data);
}

void FBXRenderer::SetDiffuse(Color color)
{
    diffuseFactor = { color.R(), color.G(), color.B() };

    for (auto& material : fbxData->GetMesh())
        material.GetMaterial().diffuseFactor = diffuseFactor;
}

void FBXRenderer::SetAlpha(float value)
{
    alphaFactor = value;
    for (auto& material : fbxData->GetMesh())
        material.GetMaterial().alphaFactor = alphaFactor;
}

void FBXRenderer::SetEmissive(Color color)
{
    emissiveFactor = { color.R(), color.G(), color.B() };

    for (auto& material : fbxData->GetMesh())
        material.GetMaterial().emissiveFactor = emissiveFactor;
}

void FBXRenderer::SetRoughness(float value)
{
    float factor = std::clamp(value, 0.0f, 1.0f);
    roughnessFactor = factor;

    for (auto& material : fbxData->GetMesh())
        material.GetMaterial().roughnessFactor = factor;
}

void FBXRenderer::SetMatalic(float value)
{
    float factor = std::clamp(value, 0.0f, 1.0f);
    metalicFactor = factor;

    for (auto& material : fbxData->GetMesh())
        material.GetMaterial().metallicFactor = factor;
}

void FBXRenderer::SetUseDiffuseOverride(bool flag)
{
    useDiffuseOverride = flag;
    for (auto& material : fbxData->GetMesh())
        material.GetMaterial().usediffuseOverride = useDiffuseOverride;
}

void FBXRenderer::SetUseEmissiveOverride(bool flag)
{
    useEmissiveOverride = flag;
    for (auto& material : fbxData->GetMesh())
        material.GetMaterial().useEmissiveOverride = useEmissiveOverride;
}

void FBXRenderer::SetUseRoughnessOverride(bool flag)
{
    useRoughnessOverride = flag;
    for (auto& material : fbxData->GetMesh())
        material.GetMaterial().useRoughnessOverride = useRoughnessOverride;
}

void FBXRenderer::SetUseMatalicOverride(bool flag)
{
    useMetallicOverride = flag;
    for (auto& material : fbxData->GetMesh())
        material.GetMaterial().useMetallicOverride = useMetallicOverride;
}

void FBXRenderer::SetDiffuseOverride(Color color)
{
    diffuseOverride = { color.R(), color.G(), color.B() };
    for (auto& material : fbxData->GetMesh())
        material.GetMaterial().diffuseOverride = diffuseOverride;
}

void FBXRenderer::SetEmissiveOverride(Color color)
{                            	
    emissiveOverride = { color.R(), color.G(), color.B() };
    for (auto& material : fbxData->GetMesh())
        material.GetMaterial().emissiveOverride = emissiveOverride;
}

void FBXRenderer::SetMetallicOverride(float value)
{
    metallicOverride = value;
    for (auto& material : fbxData->GetMesh())
        material.GetMaterial().metallicOverride = metallicOverride;
}

void FBXRenderer::SetRoughnessOverride(float value)
{
    roughnessOverride = value;
    for (auto& material : fbxData->GetMesh())
        material.GetMaterial().roughnessOverride = roughnessOverride;
}
