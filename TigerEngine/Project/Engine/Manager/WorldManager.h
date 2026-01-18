#pragma once
#include <pch.h>
#include <System/Singleton.h>

/// <summary>
/// [ 싱글톤 ] 
/// 게임에서 사용하는 월드 정보 데이터를 가지고 있는 매니저
/// </summary>
class WorldManager : public Singleton<WorldManager>
{
public:
	WorldManager(token) {};
	~WorldManager() = default;

	void CreateDirectionalLightFrustum();
	void Update();

    int GetCameraIndex();
    void SetCameraIndex(int index);

	// Directional Light - 햇빛
	Matrix directionalLightView{};
	Matrix directionalLightProj{};

	Viewport directionalLightViewport = { 0, 0, 8192, 8192, 0.0f, 1.0f }; // x, y, width, height, min, max
	Vector4 lightDirection{ 0, -1, 0, 1 };

	Vector3 directionalLightLookAt{};
	Vector3 directionalLightPos{};
	Vector3 directionalLightUpDistFromLookAt{ 1000, 1000, 1000 };

	float directionalLightFrustumAngle = XM_PIDIV4;
	float directionalLightForwardDistFromCamera = 1.0f;

	float directionalLightNear = 400.0f;
	float directionalLightFar = 3000.0f;

	const float directionalLightMinNear = 400.0f;
	const float directionalLightMinFar = 1001.0f;

	ComPtr<ID3D11ShaderResourceView> shaderResourceView{};

private:
    int cameraIndex = 0;
};