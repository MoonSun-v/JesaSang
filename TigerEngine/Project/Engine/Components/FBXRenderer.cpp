#include "FBXRenderer.h"
#include "../Manager/ComponentFactory.h"
#include "../Scene/Scene.h"
#include "../Components/FBXData.h"
#include "../Manager/ShaderManager.h"
#include "../Object/GameObject.h"
#include "../Util/JsonHelper.h"

RTTR_REGISTRATION
{
	rttr::registration::class_<FBXRenderer>("FBXRenderer")
		.constructor<>()
			(rttr::policy::ctor::as_std_shared_ptr)
		.property("AnimationIndex", 	&FBXRenderer::GetAnimationIndex,			&FBXRenderer::SetAnimationIndex)
		.property("AnimationPlayTime", 	&FBXRenderer::GetProgressAnimationTime,		&FBXRenderer::SetProgressAnimationTime)
		.property("IsAnimationPlay", 	&FBXRenderer::GetIsAnimationPlay,			&FBXRenderer::SetIsAnimationPlay)
		.property("Roughness", 			&FBXRenderer::GetRoughness,					&FBXRenderer::SetRoughness)
		.property("Metalic", 			&FBXRenderer::GetMatalic,					&FBXRenderer::SetMatalic)
		.property("Color", 				&FBXRenderer::GetColor,						&FBXRenderer::SetColor);
        
         // TODO :: material data 추가

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
	bones.clear();
	if(fbxData != nullptr) CreateBoneInfo(); 
}

void FBXRenderer::OnStart()
{
    cout << "RenderComponent_OnStart() : FBXRenderer OnStart() check...\n"; // RenderComponent OnStart 체크
}

void FBXRenderer::OnUpdate(float delta)
{
	if (fbxData == nullptr) return;
    if (fbxData->GetFBXInfo()->type == ModelType::Static) return;

    auto modelAsset = fbxData->GetFBXInfo();
    if (!modelAsset->animations.empty() && isAnimPlay)
	{
		progressAnimationTime += delta;
		progressAnimationTime = fmod(progressAnimationTime, modelAsset->animations[animationIndex].m_duration);
	}

    // local & model matrix udpate
    switch (modelAsset->type)
    {
    case ModelType::Skeletal:
        for (auto& bone : bones)
        {
            // animation update
            if (bone.m_nodeAnimation.m_nodeName != "" &&
                (!bone.m_nodeAnimation.Positions.empty() ||
                    !bone.m_nodeAnimation.Rotations.empty() ||
                    !bone.m_nodeAnimation.Scales.empty()))

            {
                Vector3 positionVec = Vector3::Zero;
                Vector3 scaleVec = Vector3::One;
                Quaternion rotationQuat = Quaternion::Identity;
                bone.m_nodeAnimation.Evaluate(progressAnimationTime, positionVec, rotationQuat, scaleVec);

                // bone local update
                Matrix mat = Matrix::CreateScale(scaleVec) * Matrix::CreateFromQuaternion(rotationQuat) * Matrix::CreateTranslation(positionVec);
                bone.m_localTransform = mat.Transpose();
            }

            // bone world udpate
            if (bone.m_parentIndex != -1)
                bone.m_worldTransform = bones[bone.m_parentIndex].m_worldTransform * bone.m_localTransform;
            else
                bone.m_worldTransform = bone.m_localTransform;

            // bone pose arr update
            bonePoses.bonePose[bone.m_index] = bone.m_worldTransform;
        }
        break;

    case ModelType::Rigid:
        // local matrix
        if (modelAsset->animations.empty()) {
            for (int i = 0; i < modelAsset->meshes_modelMat.size(); i++)
                modelAsset->meshes_localMat[i] = modelAsset->meshes_bindMat[i];
        }
        else {
            int nodeCount = modelAsset->meshes.size();
            for (int i = 0; i < nodeCount; i++)
            {
                // node animation find
                auto& node = modelAsset->meshes[i];
                NodeAnimation aniClip;
                bool hasAnimation = modelAsset->animations[animationIndex].GetNodeAnimationByName(node.nodeName, aniClip);
                
                // animation keyframe local
                if (hasAnimation)
                {
                    // get keyframe
                    Vector3 pos;  Quaternion rot;	Vector3 scl;
                    aniClip.Evaluate(progressAnimationTime, pos, rot, scl);

                    modelAsset->meshes_localMat[i] = Matrix::CreateScale(scl) *
                        Matrix::CreateFromQuaternion(rot) *
                        Matrix::CreateTranslation(pos);
                }
                // bind local
                else
                    modelAsset->meshes_localMat[i] = modelAsset->meshes_bindMat[i];
            }
        }

        // model matrix
        for (int i = 0; i < modelAsset->meshes_modelMat.size(); i++)
        {
            auto& sub = modelAsset->meshes[i];

            if (sub.parentIndex > -1)
                modelAsset->meshes_modelMat[i] = modelAsset->meshes_localMat[i] * modelAsset->meshes_modelMat[sub.parentIndex];
            else
                modelAsset->meshes_modelMat[i] = modelAsset->meshes_localMat[i];
        }
        break;

    default:
        // static has none model matrix
        // static has none animation
        break;
    }
}

void FBXRenderer::OnDestory()
{
}

void FBXRenderer::OnRender(RenderQueue& queue)
{
    if (fbxData == nullptr) return; // 그릴 메쉬가 없음 -> data 없음

    ModelType modelType = fbxData->GetFBXInfo()->type;

    auto& meshData = fbxData->GetMesh();
    auto world = owner->GetTransform()->GetWorldTransform();

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

nlohmann::json FBXRenderer::Serialize()
{
    return JsonHelper::MakeSaveData(this);
}

void FBXRenderer::Deserialize(nlohmann::json data)
{
	// data : data["objects"]["properties"]["components"]["현재 컴포넌트"]

    JsonHelper::SetDataFromJson(this, data);
}

void FBXRenderer::SetRoughness(float value)
{
    float factor = std::clamp(value, 0.0f, 1.0f);
    roughness = factor;

    for (auto& material : fbxData->GetMesh())
    {
        material.GetMaterial().roughnessFactor = factor;
    }
}

void FBXRenderer::SetMatalic(float value)
{
    float factor = std::clamp(value, 0.0f, 1.0f);
    metalic = factor;

    for (auto& material : fbxData->GetMesh())
    {
        material.GetMaterial().metallicFactor = factor;
    }
}

void FBXRenderer::SetColor(Color value)
{
    for (auto& material : fbxData->GetMesh())
    {
        // material.GetMaterial().diffuseOverride = color;
    }

    color = value;
}

void FBXRenderer::CreateBoneInfo()
{
    auto modelAsset = fbxData->GetFBXInfo();
    int size = modelAsset->skeletalInfo.m_bones.size();
	for (int i = 0; i < size; i++)
	{
		string boneName = modelAsset->skeletalInfo.m_bones[i].name;
		BoneInfo boneInfo = modelAsset->skeletalInfo.GetBoneInfoByName(boneName);
		int boneIndex = modelAsset->skeletalInfo.GetBoneIndexByName(boneName);

		string parentBoneName = boneInfo.parentBoneName;
		BoneInfo parentBoneInfo;
		int parentBoneIndex = -1;
		if (parentBoneName != "")
		{
			parentBoneInfo = modelAsset->skeletalInfo.GetBoneInfoByName(parentBoneName);
			parentBoneIndex = modelAsset->skeletalInfo.GetBoneIndexByName(parentBoneName);
		}

		Matrix localMat = boneInfo.relativeTransform;
		Matrix worldMat = parentBoneIndex != -1 ? bones[parentBoneIndex].m_worldTransform * localMat : localMat;

		// Bone 정보 생성
		Bone bone;
		bone.CreateBone(boneName, parentBoneIndex, boneIndex, worldMat, localMat);	//...

		NodeAnimation boneAnim;
		bool hasAnim = !modelAsset->animations.empty();
		if (hasAnim)
		{
			if (modelAsset->animations[animationIndex].GetNodeAnimationByName(boneName, boneAnim))
			{
				bone.m_nodeAnimation = boneAnim;
			}
		}

		bones.push_back(bone);
	}
}