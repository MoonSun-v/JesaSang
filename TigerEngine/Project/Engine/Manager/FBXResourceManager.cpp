#include "FBXResourceManager.h"

#include <assimp\Importer.hpp>
#include <assimp\scene.h>
#include <assimp\postprocess.h>
#include <DirectXTex.h>
#include <algorithm>
#include <wincodec.h>
#pragma comment(lib, "windowscodecs.lib")

inline Matrix ToSimpleMathMatrix(const aiMatrix4x4& m)
{
	return Matrix(
		m.a1, m.b1, m.c1, m.d1,
		m.a2, m.b2, m.c2, m.d2,
		m.a3, m.b3, m.c3, m.d3,
		m.a4, m.b4, m.c4, m.d4
	);
}

void FBXResourceManager::ProcessSkeletalNode(std::shared_ptr<FBXResourceAsset>& pAsset, aiNode* pNode, const aiScene* pScene)
{
	string boneName = pNode->mName.C_Str();
	BoneInfo boneInfo = pAsset->skeletalInfo.GetBoneInfoByName(boneName);
	int boneIndex = pAsset->skeletalInfo.GetBoneIndexByName(boneName);

	// node 추적
	for (UINT i = 0; i < pNode->mNumMeshes; i++)
	{
		aiMesh* mesh = pScene->mMeshes[pNode->mMeshes[i]];
		pAsset->meshes.push_back(this->ProcessMeshTexture(pAsset, mesh, pScene));
		pAsset->meshes.back().refBoneIndex = boneIndex;

		pAsset->meshes.back().SetMaterial(pScene->mMaterials[mesh->mMaterialIndex]);
        
		// weight 저장
		ProcessBoneWeight(pAsset, mesh);
	}

	for (UINT i = 0; i < pNode->mNumChildren; i++)
	{
		this->ProcessSkeletalNode(pAsset, pNode->mChildren[i], pScene);
	}
}

void FBXResourceManager::ProcessRigidNode(std::shared_ptr<FBXResourceAsset>& pAsset, aiNode* pNode, const aiScene* pScene, int parentIndex)
{
    if (!pAsset || !pNode || !pScene) return;

    // bind
    auto& bindMats = pAsset->meshes_bindMat;

    // sub mesh (+ texture)
    for (UINT i = 0; i < pNode->mNumMeshes; ++i)
    {
        aiMesh* mesh = pScene->mMeshes[pNode->mMeshes[i]];
        pAsset->meshes.push_back(this->ProcessMeshTexture(pAsset, mesh, pScene));
       
        Mesh& outMesh = pAsset->meshes.back();
        outMesh.parentIndex = parentIndex - 1;
        outMesh.refBoneIndex = -1;
        outMesh.nodeName = pNode->mName.C_Str();
        bindMats.push_back(XMMatrixTranspose(XMLoadFloat4x4((XMFLOAT4X4*)&pNode->mTransformation)));
    }

    // cur node index
    const int myIndex = static_cast<int>(pAsset->meshes.size());
    // child node
    for (UINT c = 0; c < pNode->mNumChildren; ++c)
    {
        this->ProcessRigidNode(pAsset, pNode->mChildren[c], pScene, myIndex);
    }
}

Mesh FBXResourceManager::ProcessMeshTexture(std::shared_ptr<FBXResourceAsset>& pAsset, aiMesh* pMesh, const aiScene* pScene)
{
	// Data to fill
	std::vector<BoneWeightVertexData> vertices;
	std::vector<UINT> indices;
	std::vector<Texture> textures;

	// Walk through each of the mesh's vertices
	for (UINT i = 0; i < pMesh->mNumVertices; i++)
	{
		BoneWeightVertexData vertex{};

		vertex.position = { pMesh->mVertices[i].x, pMesh->mVertices[i].y,  pMesh->mVertices[i].z };
		vertex.normal = { pMesh->mNormals[i].x, pMesh->mNormals[i].y, pMesh->mNormals[i].z };
		vertex.tangent = { pMesh->mTangents[i].x, pMesh->mTangents[i].y, pMesh->mTangents[i].z };
		vertex.bitangent = { pMesh->mBitangents[i].x, pMesh->mBitangents[i].y, pMesh->mBitangents[i].z };

		if (pMesh->mTextureCoords[0]) {
			vertex.texture.x = (float)pMesh->mTextureCoords[0][i].x;
			vertex.texture.y = (float)pMesh->mTextureCoords[0][i].y;
		}

		pAsset->boxMin = Vector3::Min(pAsset->boxMin, vertex.position);
		pAsset->boxMax = Vector3::Max(pAsset->boxMax, vertex.position);

		vertices.push_back(vertex);
	}

	// face 인덱스 저장
	for (UINT i = 0; i < pMesh->mNumFaces; i++)
	{
		aiFace face = pMesh->mFaces[i];
		if (face.mNumIndices != 3) continue; // 삼각형만

		for (UINT j = 0; j < face.mNumIndices; j++)
		{
			int currIndex = face.mIndices[j];
			indices.push_back(currIndex);
		}
	}

	// 머터리얼 불러오기
	if (pMesh->mMaterialIndex >= 0)
	{
		aiMaterial* material = pScene->mMaterials[pMesh->mMaterialIndex];

		// diffuseMap 불러오기
		std::vector<Texture> diffuseMaps = this->loadMaterialTextures(pAsset, material, aiTextureType_DIFFUSE, TEXTURE_DIFFUSE, pScene);
		if (!diffuseMaps.empty())
		{
			textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());
		}

		// emissiveMap 불러오기
		std::vector<Texture> emissiveMaps = this->loadMaterialTextures(pAsset, material, aiTextureType_EMISSIVE, TEXTURE_EMISSIVE, pScene);
		if (!emissiveMaps.empty())
		{
			textures.insert(textures.end(), emissiveMaps.begin(), emissiveMaps.end());
		}

		// normalMap 불러오기
		std::vector<Texture> normalMaps = this->loadMaterialTextures(pAsset, material, aiTextureType_NORMALS, TEXTURE_NORMAL, pScene);
		if (!normalMaps.empty())
		{
			textures.insert(textures.end(), normalMaps.begin(), normalMaps.end());
		}

        // roughnessMap 불러오기
        std::vector<Texture> roughneesMaps = this->loadMaterialTextures(pAsset, material, aiTextureType_DIFFUSE_ROUGHNESS, TEXTURE_ROUGHNESS, pScene);
        if (!roughneesMaps.empty())
        {
            textures.insert(textures.end(), roughneesMaps.begin(), roughneesMaps.end());
        }

        // shineness 불러오기
        std::vector<Texture> shinenessMaps = this->loadMaterialTextures(pAsset, material, aiTextureType_SHININESS, TEXTURE_SHININESS, pScene);
        if (!shinenessMaps.empty())
        {
            textures.insert(textures.end(), shinenessMaps.begin(), shinenessMaps.end());
        }

        // metallicMap 불러오기
        std::vector<Texture> metallicMaps = this->loadMaterialTextures(pAsset, material, aiTextureType_METALNESS, TEXTURE_METALNESS, pScene);
        if (!metallicMaps.empty())
        {
            textures.insert(textures.end(), metallicMaps.begin(), metallicMaps.end());
        }

		// specularMap 불러오기 (PBR had none specular)
		std::vector<Texture> sepcualrMaps = this->loadMaterialTextures(pAsset, material, aiTextureType_SPECULAR, TEXTURE_SPECULAR, pScene);
		if (!sepcualrMaps.empty())
		{
			textures.insert(textures.end(), sepcualrMaps.begin(), sepcualrMaps.end());
		}
	}

	return Mesh(device, vertices, indices, textures);
}

void FBXResourceManager::ProcessBoneWeight(std::shared_ptr<FBXResourceAsset>& pAsset, aiMesh* pMesh)
{
	UINT meshBoneCount = pMesh->mNumBones;
	if (meshBoneCount <= 0) return;

	for (UINT i = 0; i < meshBoneCount; i++)
	{
		aiBone* pAiBone = pMesh->mBones[i];
		UINT boneIndex = pAsset->skeletalInfo.GetBoneIndexByName(pAiBone->mName.C_Str());
		BoneInfo bone = pAsset->skeletalInfo.GetBoneInfoByIndex(boneIndex);

		for (UINT j = 0; j < pAiBone->mNumWeights; j++)
		{
			UINT vertexId = pAiBone->mWeights[j].mVertexId;
			float weight = pAiBone->mWeights[j].mWeight;
			pAsset->meshes.back().vertices[vertexId].AddBoneData(boneIndex, weight);
		}
	}
}

std::vector<Texture> FBXResourceManager::loadMaterialTextures(std::shared_ptr<FBXResourceAsset>& pAsset, aiMaterial* mat, aiTextureType type, std::string typeName, const aiScene* scene)
{
    std::vector<Texture> textures;
    auto& textureloadeds = pAsset->textures;

    for (UINT i = 0; i < mat->GetTextureCount(type); i++)
    {
        aiString str;
        mat->GetTexture(type, i, &str);

        bool skip = false;
        for (UINT j = 0; j < textureloadeds.size(); j++)
        {
            if (std::strcmp(textureloadeds[j].path.c_str(), str.C_Str()) == 0)
            {
                textures.push_back(textureloadeds[j]);
                skip = true;
                break;
            }
        }
        if (skip) continue;

        // texture file name
        Texture texture;
        const std::string texRef = str.C_Str();
        const std::string filenameOnly = std::filesystem::path(texRef).filename().string();


        // texture file path
        std::string fullpath = pAsset->directory + "\\" + filenameOnly;

        // 내장 텍스처 save
        if (scene && scene->GetEmbeddedTexture(str.C_Str()))
        {
            SaveEmbeddedTextureIfExists(scene, pAsset->directory, filenameOnly, &fullpath);
        }

        std::wstring wfullpath(fullpath.begin(), fullpath.end());

        // texture load (SRGB / Linear)
        if (typeName == TEXTURE_DIFFUSE)
            CreateTextureFromFile(device.Get(), wfullpath.c_str(), texture.pTexture.GetAddressOf(), TextureColorSpace::SRGB);
        else
            CreateTextureFromFile(device.Get(), wfullpath.c_str(), texture.pTexture.GetAddressOf(), TextureColorSpace::LINEAR);

        // push
        texture.type = typeName;
        texture.path = str.C_Str();
        textures.push_back(texture);
        textureloadeds.push_back(texture);
    }

    return textures;
}

// 임베디드 텍스처 참조하는지 확인
static bool IsEmbeddedRef(const std::string& s) { return !s.empty() && s[0] == '*'; }

// 임베디드 텍스처 인덱스 반환 함수
static bool TryParseEmbeddedIndex(const std::string& s, int& outIdx) {
    if (!IsEmbeddedRef(s)) return false;
    try {
        outIdx = std::stoi(s.substr(1)); // *0 -> 숫자부분
        return true;
    }
    catch (...) {
        return false;
    }
}

bool FBXResourceManager::SaveEmbeddedTextureIfExists(const aiScene* scene, const std::string& directory,
          const std::string& filename, std::string* outSavedPath)
{
    if (!scene) return false;

    const aiTexture* embedded = scene->GetEmbeddedTexture(filename.c_str());
    if (!embedded) return false;

    std::string dir = directory;
    if (!dir.empty() && dir.back() != '\\' && dir.back() != '/')
        dir += "\\";

    // *0 내용이 있으면 파싱
    std::string saveName = filename;
    int idx = -1;
    if (TryParseEmbeddedIndex(filename, idx)) 
    {
        // 압축이면 포맷 힌트로 확장자 결정 가능
        std::string ext = "png";
        if (embedded->mHeight == 0 && embedded->achFormatHint[0] != '\0')
            ext = embedded->achFormatHint;   // "png" / "jpg" 등

        std::string name = embedded->mFilename.C_Str();
        saveName = name + "." + ext; // ??
        *outSavedPath = dir + saveName;
        return true;
    }

    const std::string fullPath = dir + saveName; // fullPath 갱신

    // 이미 있으면 리턴
    std::ifstream test(fullPath, std::ios::binary);
    if (test.good()) // 해당 파일이 존재하면
    {
        if (outSavedPath)
        {
            *outSavedPath = fullPath;
        }
        return true;
    }

    // 1) 압축 텍스처
    if (embedded->mHeight == 0) {
        std::ofstream file(fullPath, std::ios::binary);
        if (!file.is_open()) return false;

        file.write(reinterpret_cast<const char*>(embedded->pcData), embedded->mWidth);
        file.close();

        if (outSavedPath) *outSavedPath = fullPath;
        return true;
    }

    // 2) 비압축 텍스처 (주의: embedded->pcData는 aiTexel(RGBA)일 가능성 큼)
    return SaveBGRA8ToPNG_WIC(
        fullPath,
        embedded->mWidth,
        embedded->mHeight,
        reinterpret_cast<const uint8_t*>(embedded->pcData),
        outSavedPath
    );
}

bool FBXResourceManager::SaveBGRA8ToPNG_WIC(const std::string& fullPath, uint32_t width, uint32_t height,
    const uint8_t* bgraPixels, std::string* outSavedPath)
{
    Microsoft::WRL::ComPtr<IWICImagingFactory> factory;
    HRESULT hr = CoCreateInstance(
        CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER,
        IID_PPV_ARGS(factory.GetAddressOf())
    );
    if (FAILED(hr)) return false;

    Microsoft::WRL::ComPtr<IWICStream> stream;
    hr = factory->CreateStream(stream.GetAddressOf());
    if (FAILED(hr)) return false;

    std::wstring wpath(fullPath.begin(), fullPath.end());
    hr = stream->InitializeFromFilename(wpath.c_str(), GENERIC_WRITE);
    if (FAILED(hr)) return false;

    Microsoft::WRL::ComPtr<IWICBitmapEncoder> encoder;
    hr = factory->CreateEncoder(GUID_ContainerFormatPng, nullptr, encoder.GetAddressOf());
    if (FAILED(hr)) return false;

    hr = encoder->Initialize(stream.Get(), WICBitmapEncoderNoCache);
    if (FAILED(hr)) return false;

    Microsoft::WRL::ComPtr<IWICBitmapFrameEncode> frame;
    Microsoft::WRL::ComPtr<IPropertyBag2> props;
    hr = encoder->CreateNewFrame(frame.GetAddressOf(), props.GetAddressOf());
    if (FAILED(hr)) return false;

    hr = frame->Initialize(props.Get());
    if (FAILED(hr)) return false;

    hr = frame->SetSize(width, height);
    if (FAILED(hr)) return false;

    WICPixelFormatGUID format = GUID_WICPixelFormat32bppBGRA;
    hr = frame->SetPixelFormat(&format);
    if (FAILED(hr)) return false;

    const UINT stride = width * 4;
    const UINT imageSize = stride * height;
    hr = frame->WritePixels(height, stride, imageSize, const_cast<BYTE*>(bgraPixels));
    if (FAILED(hr)) return false;

    hr = frame->Commit();
    if (FAILED(hr)) return false;

    hr = encoder->Commit();
    if (FAILED(hr)) return false;

    if (outSavedPath) *outSavedPath = fullPath;
    return true;
}

void FBXResourceManager::GetDevice(const ComPtr<ID3D11Device> &pDevice, const ComPtr<ID3D11DeviceContext> &pDeviceContext)
{
	this->device = pDevice;
	this->deviceContext = pDeviceContext;
}

std::shared_ptr<FBXResourceAsset> FBXResourceManager::LoadFBXByPath(std::string path)
{
	// map에 먼저 있는지 확인
	auto it = assets.find(path);

	if (it != assets.end()) // 존재함
	{
		if (!it->second.expired())
		{
			shared_ptr<FBXResourceAsset> assetPtr = it->second.lock();
			return assetPtr;
		}
		else
		{
			assets.erase(it); // 지우고 새로 만들기
		}
	}

	Assimp::Importer importer;

	unsigned int importFlag = aiProcess_Triangulate |	// 삼각형 변환
		aiProcess_GenNormals |				// 노말 생성
		aiProcess_GenUVCoords |				// UV 생성
		aiProcess_CalcTangentSpace |		// 탄젠트 생성
		aiProcess_LimitBoneWeights |		// 본의 영향을 받는 정점의 최대 개수 4개로 제한
		aiProcess_GenBoundingBoxes |		// Bounding box 만들기
		aiProcess_ConvertToLeftHanded;		// 왼손 좌표계로 변환

	importer.SetPropertyBool(AI_CONFIG_IMPORT_FBX_PRESERVE_PIVOTS, 0);

	const aiScene* pScene = importer.ReadFile(path, importFlag);

	if (pScene == nullptr) return std::shared_ptr<FBXResourceAsset>();

	// 없으면 load후 map에 추가 
	auto sharedAsset = make_shared<FBXResourceAsset>();

	if (pScene == nullptr)
		return nullptr;

	sharedAsset ->directory = path.substr(0, path.find_last_of("/\\"));

	// skeletonInfo 저장
	sharedAsset ->skeletalInfo = SkeletonInfo();
	sharedAsset ->skeletalInfo.CreateFromAiScene(pScene);

	// animation 저장
	int animationsNum = pScene->mNumAnimations;
	for (int i = 0; i < animationsNum; i++)
	{
		Animation anim;
		anim.CreateNodeAnimation(pScene->mAnimations[i]);
		sharedAsset->animations.push_back(anim);
	}

	// Process Node (mesh, meterial/textures)
    // 1) Skeletal Mesh
    if (sharedAsset->skeletalInfo.IsSkeletal()) {
        // mesh type
        sharedAsset->type = ModelType::Skeletal;

        // mesh, material(texture)
        ProcessSkeletalNode(sharedAsset, pScene->mRootNode, pScene);

        for (auto& bone : sharedAsset->skeletalInfo.m_bones)
        {
            Matrix offsetMat = Matrix::Identity;
            std::string currBoneName = bone.name;
            int boneIndex = sharedAsset->skeletalInfo.GetBoneIndexByName(currBoneName);

            if (boneIndex > 0)
                offsetMat = sharedAsset->skeletalInfo.GetBoneOffsetByName(currBoneName);

            sharedAsset->m_BoneOffsets.boneOffset[boneIndex] = offsetMat;
        }
    }
    // 2) Static / Rigid Mesh
    else {
        // mesh type
        sharedAsset->type = ModelType::Rigid;       // TODO :: Static / Rigid 구분 필요

        // mesh, material(texture)
        ProcessRigidNode(sharedAsset, pScene->mRootNode, pScene, -1);
        sharedAsset->meshes_modelMat.resize(sharedAsset->meshes.size());
        sharedAsset->meshes_localMat.resize(sharedAsset->meshes.size());
    }

    // create vertex/index buffer
    for (auto& mesh : sharedAsset->meshes)
    {
        mesh.CreateVertexBuffer(device);
        mesh.CreateIndexBuffer(device);
    }

    // 피킹 aabb
    if (pScene->mRootNode->mNumChildren > 0) 
    {
        if (pScene->HasAnimations()) // gltf 파일이 RootNode->mChildren[0] 이 nullptr
        {
            // auto firstAnimKey = sharedAsset->animations[0].m_boneAnimations[0].m_keys[0];
            // Matrix mat = Matrix::CreateScale(firstAnimKey.m_scale) * 
            // 			Matrix::CreateFromQuaternion(firstAnimKey.m_rotation) * 
            // 			Matrix::CreateTranslation(firstAnimKey.m_position);
            // sharedAsset->boxMin = Vector3::Transform(sharedAsset->boxMin, mat);
            // sharedAsset->boxMax = Vector3::Transform(sharedAsset->boxMax, mat);

            // NOTE : 애니메이션 박스들은 절반씩 위로 올려줌 ( 모델이랑 어긋나서 하드 코딩 )
            sharedAsset->boxMin = Vector3::Transform(sharedAsset->boxMin, ToSimpleMathMatrix(pScene->mRootNode->mChildren[0]->mTransformation));
            sharedAsset->boxMax = Vector3::Transform(sharedAsset->boxMax, ToSimpleMathMatrix(pScene->mRootNode->mChildren[0]->mTransformation));
            sharedAsset->boxCenter = { 0.0f, (sharedAsset->boxMax - sharedAsset->boxMin).y / 2.0f, 0.0f };
        }
        else
        {
            sharedAsset->boxMin = Vector3::Transform(sharedAsset->boxMin, ToSimpleMathMatrix(pScene->mRootNode->mChildren[0]->mTransformation));
            sharedAsset->boxMax = Vector3::Transform(sharedAsset->boxMax, ToSimpleMathMatrix(pScene->mRootNode->mChildren[0]->mTransformation));
            sharedAsset->boxCenter = Vector3::Zero;
        }
    }


	// map에 저장하기
	weak_ptr<FBXResourceAsset> weakAsset = sharedAsset;
	assets.insert({ path, weakAsset });

	return sharedAsset;
}
