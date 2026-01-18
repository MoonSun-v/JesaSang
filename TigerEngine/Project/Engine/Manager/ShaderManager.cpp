#include "ShaderManager.h"
#include <Datas/Vertex.h>
#include <Datas/FBXResourceData.h>
#include <Datas/TransformData.h>

struct CameraData
{
    Matrix view;
    Matrix projection;
    Matrix positionWS;

    Matrix shadowView;
    Matrix shadowProjection;

    Vector4 ambient;	// 환경광
    Vector4 diffuse;	// 난반사
    Vector4 specular;	// 정반사
    FLOAT shininess;	// 광택지수
    Vector3 CameraPos;	// 카메라 위치
};

struct DirectionalLightData
{
    Vector4 lightDirection;
    Vector4 lightColor;
};

void ShaderManager::CreateCB(const ComPtr<ID3D11Device> &dev)
{
    D3D11_BUFFER_DESC mbd = {};
	mbd.Usage = D3D11_USAGE_DEFAULT;
	mbd.ByteWidth = sizeof(CameraData);
	mbd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	mbd.CPUAccessFlags = 0;
	HR_T(dev->CreateBuffer(&mbd, nullptr, cameraCB.GetAddressOf()));

    mbd = {};
	mbd.Usage = D3D11_USAGE_DEFAULT;
	mbd.ByteWidth = sizeof(MaterialData);
	mbd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	mbd.CPUAccessFlags = 0;
	HR_T(dev->CreateBuffer(&mbd, nullptr, materialCB.GetAddressOf()));
    
    mbd = {};
	mbd.Usage = D3D11_USAGE_DEFAULT;
	mbd.ByteWidth = sizeof(TransformData);
	mbd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	mbd.CPUAccessFlags = 0;
	HR_T(dev->CreateBuffer(&mbd, nullptr, transformCB.GetAddressOf()));

    mbd = {};
	mbd.Usage = D3D11_USAGE_DEFAULT;
	mbd.ByteWidth = sizeof(BonePoseBuffer);
	mbd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	mbd.CPUAccessFlags = 0;
	HR_T(dev->CreateBuffer(&mbd, nullptr, bonePoseCB.GetAddressOf()));

    mbd = {};
	mbd.Usage = D3D11_USAGE_DEFAULT;
	mbd.ByteWidth = sizeof(BoneOffsetBuffer);
	mbd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	mbd.CPUAccessFlags = 0;
	HR_T(dev->CreateBuffer(&mbd, nullptr, boneOffsetCB.GetAddressOf()));

    mbd = {};
	mbd.Usage = D3D11_USAGE_DEFAULT;
	mbd.ByteWidth = sizeof(DirectionalLightData);
	mbd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	mbd.CPUAccessFlags = 0;
	HR_T(dev->CreateBuffer(&mbd, nullptr, directionalLightCB.GetAddressOf()));
}

ComPtr<ID3D11Buffer> &ShaderManager::GetCameraCB()
{
    return cameraCB;
}

ComPtr<ID3D11Buffer> &ShaderManager::GetMaterialCB()
{
    return materialCB;
}

ComPtr<ID3D11Buffer> &ShaderManager::GetTransformCB()
{
    return transformCB;
}

ComPtr<ID3D11Buffer> &ShaderManager::GetBonePoseCB()
{
    return bonePoseCB;
}

ComPtr<ID3D11Buffer> &ShaderManager::GetBoneOffsetCB()
{
    return boneOffsetCB;
}

ComPtr<ID3D11Buffer> &ShaderManager::GetDirectionalLightCB()
{
    return directionalLightCB;
}
