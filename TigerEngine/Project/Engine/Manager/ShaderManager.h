#pragma once
#include <pch.h>
#include <System/Singleton.h>

/// @brief 셰이더에 사용하는 데이터를 가지고 있는 클래스
/// @date 26.01.03
class ShaderManager : public Singleton<ShaderManager>
{
public:
    ShaderManager(token) {};
    ~ShaderManager() = default;

    void CreateCB(const ComPtr<ID3D11Device>& dev);

    // CB Getter
    ComPtr<ID3D11Buffer>& GetFrameCB();
    ComPtr<ID3D11Buffer>& GetTransformCB();
    ComPtr<ID3D11Buffer>& GetLightingCB();
    ComPtr<ID3D11Buffer>& GetMaterialCB();
    ComPtr<ID3D11Buffer>& GetOffsetMatrixCB();
    ComPtr<ID3D11Buffer>& GetPoseMatrixCB();
    ComPtr<ID3D11Buffer>& GetPostProcessCB();
    ComPtr<ID3D11Buffer>& GetBloomCB();
    ComPtr<ID3D11Buffer>& GetEffectCB();


private:
    /* ---------------------------- constant Buffers ---------------------------- */
    ComPtr<ID3D11Buffer> frameCB;
    ComPtr<ID3D11Buffer> transformCB;
    ComPtr<ID3D11Buffer> lightingCB;
    ComPtr<ID3D11Buffer> materialCB;
    ComPtr<ID3D11Buffer> offsetMatrixCB;
    ComPtr<ID3D11Buffer> poseMatrixCB;
    ComPtr<ID3D11Buffer> postProcessCB;
    ComPtr<ID3D11Buffer> bloomCB;
    ComPtr<ID3D11Buffer> effectCB;
};