#include "FBXRenderer.h"
#include "System/ComponentFactory.h"
#include "Scene/Scene.h"
#include "../Components/FBXData.h"
#include "../Manager/ShaderManager.h"
#include "Datas/TransformData.h"

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
	if(fbxData != nullptr) CreateBoneInfo(); // 임시

    bonePoseCB      = ShaderManager::Instance().GetBonePoseCB();
    boneOffsetCB    = ShaderManager::Instance().GetBoneOffsetCB();
    materialCB      = ShaderManager::Instance().GetMaterialCB();
    transformCB     = ShaderManager::Instance().GetTransformCB();
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

	// pose 본 갱신
	for (auto& bone : bones)
	{
		// 애니메이션 업데이트
		if (bone.m_boneAnimation.m_boneName != "" && bone.m_boneAnimation.m_keys.size() >= 1)
		{
			Vector3 positionVec = Vector3::Zero;
			Vector3 scaleVec = Vector3::One;
			Quaternion rotationQuat = Quaternion::Identity;
			bone.m_boneAnimation.Evaluate(progressAnimationTime, positionVec, rotationQuat, scaleVec);

			// 애니메이션이 있으면 항상 transform 갱신 (기본값이 아닐 수도 있음)
			Matrix mat = Matrix::CreateScale(scaleVec) * Matrix::CreateFromQuaternion(rotationQuat) * Matrix::CreateTranslation(positionVec);
			bone.m_localTransform = mat.Transpose();
		}

		// 위치 갱신
		if (bone.m_parentIndex != -1)
		{
			bone.m_worldTransform = bones[bone.m_parentIndex].m_worldTransform * bone.m_localTransform;
		}
		else
		{
			bone.m_worldTransform = bone.m_localTransform;
		}

		bonePoses.modelMatricies[bone.m_index] = bone.m_worldTransform;
	}	
}

void FBXRenderer::OnDestory()
{
}

void FBXRenderer::OnRender(ComPtr<ID3D11DeviceContext>& context)
{
    if (fbxData == nullptr) return; // 그릴 메쉬가 없음 -> data 없음

    auto& meshData = fbxData->GetMesh();

    context->UpdateSubresource(bonePoseCB.Get(), 0, nullptr, &bonePoses, 0, 0);
    context->UpdateSubresource(boneOffsetCB.Get(), 0, nullptr, &fbxData->GetFBXInfo()->m_BoneOffsets, 0, 0);
    context->VSSetConstantBuffers(3, 1, bonePoseCB.GetAddressOf());
    context->VSSetConstantBuffers(4, 1, boneOffsetCB.GetAddressOf());

    TransformData tb = {};
    tb.isRigid = fbxData->GetFBXInfo()->skeletalInfo.IsRigid();
    tb.world = XMMatrixTranspose(owner->GetTransform()->GetWorldTransform());

    int size = meshData.size();
    for (size_t i = 0; i < size; i++)
    {
        MaterialData meshMaterial = meshData[i].GetMaterial();

        // NOTE : 260104 FBXRenderer의 매개변수인 roughness와 matalic을 반영한다 -> 모든 메쉬가 다 변함
        meshMaterial.Roughness = roughness;
        meshMaterial.Matalness = metalic;
        meshMaterial.ambient = color;

        context->UpdateSubresource(materialCB.Get(), 0, nullptr, &meshMaterial, 0, 0);
        tb.refBoneIndex = meshData[i].refBoneIndex;

        context->UpdateSubresource(transformCB.Get(), 0, nullptr, &tb, 0, 0);
        context->VSSetConstantBuffers(2, 1, transformCB.GetAddressOf());
        context->PSSetConstantBuffers(1, 1, materialCB.GetAddressOf());
        meshData[i].Draw(context);
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

		BoneAnimation boneAnim;
		bool hasAnim = !modelAsset->animations.empty();
		if (hasAnim)
		{
			if (modelAsset->animations[animationIndex].GetBoneAnimationByName(boneName, boneAnim))
			{
				bone.m_boneAnimation = boneAnim;
			}
		}

		bones.push_back(bone);
	}
}