#include "FBXRenderer.h"
#include "../Manager/ComponentFactory.h"
#include "../Scene/Scene.h"
#include "../Components/FBXData.h"
#include "../Manager/ShaderManager.h"
#include "../Object/GameObject.h"

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
}

void FBXRenderer::OnUpdate(float delta)
{
	if(fbxData == nullptr) return;

    auto modelAsset = fbxData->GetFBXInfo();
    if (!modelAsset->animations.empty() && isAnimPlay)
	{
		progressAnimationTime += delta;
		progressAnimationTime = fmod(progressAnimationTime, modelAsset->animations[animationIndex].m_duration);
	}

    // local matrix udpate
    // skeletal mesh
    if (modelAsset->skeletalInfo.IsSkeletal())
    {
        for (auto& bone : bones)
        {
            // animation update
            if (bone.m_nodeAnimation.m_nodeName != "" && bone.m_nodeAnimation.m_keys.size() >= 1)
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
    }
    // rigid mesh
    else {
        // local udpate
        if (modelAsset->animations.empty())
        {
            for (int i = 0; i < modelAsset->meshes_modelMat.size(); i++)
                modelAsset->meshes_localMat[i] = modelAsset->meshes_bindMat[i];
        }
        else
        {
            // animation udpate
            int nodeCount = modelAsset->meshes.size();
            for (int i = 0; i < nodeCount; i++)
            {
                // get nodeAnimation
                auto& node = modelAsset->meshes[i];
                NodeAnimation aniClip;
                bool hasAnimation = modelAsset->animations[animationIndex].GetNodeAnimationByName(node.nodeName, aniClip);
                if (hasAnimation)
                {
                    // get keyframe
                    Vector3 pos;  Quaternion rot;	Vector3 scl;
                    aniClip.Evaluate(progressAnimationTime, pos, rot, scl);

                    modelAsset->meshes_localMat[i] = Matrix::CreateScale(scl) *
                        Matrix::CreateFromQuaternion(rot) *
                        Matrix::CreateTranslation(pos);
                }
                else
                    modelAsset->meshes_localMat[i] = modelAsset->meshes_bindMat[i];
            }
        }

        // model matrix udpate
        for (int i = 0; i < modelAsset->meshes_modelMat.size(); i++)
        {
            auto& sub = modelAsset->meshes[i];

            if (sub.parentIndex != -1)
                modelAsset->meshes_modelMat[i] = modelAsset->meshes_localMat[i] * modelAsset->meshes_modelMat[sub.parentIndex];
            else
                modelAsset->meshes_modelMat[i] = modelAsset->meshes_localMat[i];
        }
    }
}

void FBXRenderer::OnDestory()
{
}

void FBXRenderer::OnRender(RenderQueue& queue)
{
    if (fbxData == nullptr) return; // 그릴 메쉬가 없음 -> data 없음

    auto& meshData = fbxData->GetMesh();
    auto world = owner->GetTransform()->GetWorldTransform();

    for (int i = 0; i < meshData.size(); i++)
    {
        auto& mesh = meshData[i];

        SkeletalRenderItem item{};
        item.mesh = &mesh;
        item.world = world;
        if (!fbxData->GetFBXInfo()->skeletalInfo.IsSkeletal()) item.model = fbxData->GetFBXInfo()->meshes_modelMat[i];
        item.poses = &bonePoses;
        item.offsets = &fbxData->GetFBXInfo()->m_BoneOffsets;
        item.refBoneIndex = mesh.refBoneIndex;
        item.isSkeletal = fbxData->GetFBXInfo()->skeletalInfo.IsSkeletal();
        item.boneCount = fbxData->GetFBXInfo()->skeletalInfo.m_bones.size();

        // Mesh 기본 Material 복사
        item.material = mesh.GetMaterial();

        // 인스턴스별 override
        item.material.roughnessOverride = roughness;
        item.material.metallicOverride = metalic;
        item.material.diffuseOverride = { color.x, color.y, color.z };

        queue.AddSkeletal(item);
    }

}

nlohmann::json FBXRenderer::Serialize()
{
	nlohmann::json datas;

    rttr::type t = rttr::type::get(*this);
    datas["type"] = t.get_name().to_string();       
    datas["properties"] = nlohmann::json::object(); // 객체 생성

    for(auto& prop : t.get_properties())
    {
		std::string propName = prop.get_name().to_string();
		rttr::variant value = prop.get_value(*this);
		if (value.is_type<float>() && propName == "Roughness")
		{
			auto v = value.get_value<float>();
			datas["properties"][propName] = v;
		}
		else if (value.is_type<float>() && propName == "Metalic")
		{
			auto v = value.get_value<float>();
			datas["properties"][propName] = v;
		}
		else if (value.is_type<Color>() && propName == "Color")
		{
			auto v = value.get_value<Color>();
			datas["properties"][propName] = { v.x, v.y, v.z, v.w };
		}
	}

    return datas;
}

void FBXRenderer::Deserialize(nlohmann::json data)
{
	// data : data["objects"]["properties"]["components"]["현재 컴포넌트"]

    auto propData = data["properties"];

    rttr::type t = rttr::type::get(*this);
    for(auto& prop : t.get_properties())
    {
		std::string propName = prop.get_name().to_string();
		rttr::variant value = prop.get_value(*this);
		if (value.is_type<float>() && propName == "Roughness")
		{
			float data = propData["Roughness"];
			prop.set_value(*this, data);
		}
		else if (value.is_type<float>() && propName == "Metalic")
		{
			float data = propData["Metalic"];
			prop.set_value(*this, data);
		}
		else if (value.is_type<Color>() && propName == "Color")
		{
			Color color = { propData["Color"][0], propData["Color"][1], propData["Color"][2], propData["Color"][3] };
			prop.set_value(*this, color);
		}
	}
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