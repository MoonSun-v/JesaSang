#pragma once
#include <map>
#include <string>
#include <memory>

#include "directxtk/WICTextureLoader.h"
#include "Datas/SkeletonInfo.h"
#include "Datas/Animation.h"
#include "Datas/Mesh.h"
#include "System/Singleton.h"
#include "Datas/FBXResourceData.h"

class FBXResourceManager : public Singleton<FBXResourceManager>
{
	// 해당 매니저에서 fbx를 읽는다.
	// 이미 읽은 fbx는 map에 저장된다.
	std::map<std::string, std::weak_ptr<FBXResourceAsset>> assets;

	// texture 불러오기 위한 device, deviceContext
	ComPtr<ID3D11Device> device = nullptr;
	ComPtr<ID3D11DeviceContext> deviceContext = nullptr;

    // -------------------------------------------
	// Node Process
    void ProcessRigidNode(  shared_ptr<FBXResourceAsset>& asset, aiNode* node, const aiScene* scene, const Matrix& parent);
	void ProcessSkeletalNode(std::shared_ptr<FBXResourceAsset>& pAsset, aiNode* pNode, const aiScene* pScene);
    void ProcessStaticNode(std::shared_ptr<FBXResourceAsset>& pAsset, aiNode* pNode, const aiScene* pScene);
	
    // Mesh & Texture Save
    Mesh ProcessMeshTexture(std::shared_ptr<FBXResourceAsset>& pAsset, aiMesh* pMesh, const aiScene* pScene);

    // Bone Save
	void ProcessBoneWeight(std::shared_ptr<FBXResourceAsset>& pAsset, aiMesh* pMesh);
	
    // --------------------------------------------
    // Texture Process
    std::vector<Texture> loadMaterialTextures(std::shared_ptr<FBXResourceAsset>& pAsset, aiMaterial* mat, aiTextureType type, std::string typeName, const aiScene* scene);
    bool SaveEmbeddedTextureIfExists(const aiScene* scene, const std::string& directory,
        const std::string& filename, std::string* outSavedPath);
    bool SaveBGRA8ToPNG_WIC(const std::string& fullPath, uint32_t width, uint32_t height,
        const uint8_t* bgraPixels, std::string* outSavedPath);

public:
    FBXResourceManager(token) {};
    ~FBXResourceManager() = default;

	void GetDevice(const ComPtr<ID3D11Device>& pDevice, const ComPtr<ID3D11DeviceContext>& pDeviceContext);

    /// <summary>
    /// Rigid, Skeletal FBX 모델 가져오는 함수 ("모델" 만 가져옴. 애니메이션은 별도로 추가 해야함)
    /// </summary>
    /// <param name="path">상대 경로</param>
    std::shared_ptr<FBXResourceAsset> LoadFBXByPath(std::string path);

    /// <summary>
    /// Static FBX 모델 가져오는 함수
    /// </summary>
    /// <param name="path">상대 경로</param>
    std::shared_ptr<FBXResourceAsset> LoadStaticFBXByPath(std::string path);


    /// <summary>
    /// 애니메이션만 로드해서 기존 asset에 추가
    /// </summary>
    bool LoadAnimationByPath( std::shared_ptr<FBXResourceAsset> asset, std::string animPath );
};
