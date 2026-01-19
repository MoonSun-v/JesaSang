#pragma once
#include "../pch.h"
#include <fstream>
#include <sstream>
#include <stdexcept>
#include "../Helper.h"
#include "assimp/material.h"
#include "Vertex.h"
#include "MaterialData.h"

using namespace std;
using namespace DirectX::SimpleMath;
using namespace DirectX;
using Microsoft::WRL::ComPtr;

const string TEXTURE_DIFFUSE 	= "texture_diffuse";
const string TEXTURE_EMISSIVE 	= "texture_emissive";
const string TEXTURE_NORMAL 	= "texture_normal";
const string TEXTURE_SPECULAR 	= "texture_specular";
const string TEXTURE_METALNESS 	= "texture_metalness";
const string TEXTURE_ROUGHNESS 	= "texture_roughness";
const string TEXTURE_SHININESS	= "texture_shininess";

struct Texture
{
	string type;
	string path;

	ComPtr<ID3D11ShaderResourceView> pTexture = nullptr;
};

class Mesh
{
public:
	vector<BoneWeightVertexData> vertices;
	vector<UINT> indices;
	vector<Texture> textures;

	ComPtr<ID3D11Device> m_pDevice;
	int refBoneIndex = -1;

	Mesh(ComPtr<ID3D11Device>& dev, const std::vector<BoneWeightVertexData>& vertices, const std::vector<UINT>& indices, const std::vector<Texture>& textures) :
		vertices(vertices),
		indices(indices),
		textures(textures),
		m_pDevice(dev),
		m_pVertexBuffer(nullptr),
		m_pIndexBuffer(nullptr)
	{
		this->setupMesh();
	}

    void Draw(ComPtr<ID3D11DeviceContext>& pDeviceContext) const;
	void SetMaterial(aiMaterial* pAiMaterial);
	MaterialData& GetMaterial();
	void CreateVertexBuffer(ComPtr<ID3D11Device>& dev);
	void CreateIndexBuffer(ComPtr<ID3D11Device>& dev);
	
	//void CreateMaterialBuffer(ComPtr<ID3D11Device>& dev);

private:
	MaterialData material{};

	ComPtr<ID3D11Buffer> m_pVertexBuffer{};
	ComPtr<ID3D11Buffer> m_pIndexBuffer{};

    // Functions
    // Initializes all the buffer objects/arrays
    void setupMesh();

	void ProcessTextureByType(ComPtr<ID3D11DeviceContext>& pDeviceContext, int index) const;
};