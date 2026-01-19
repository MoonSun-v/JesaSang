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

    void Init(const ComPtr<ID3D11Device>& dev, const ComPtr<ID3D11DeviceContext>& ctx);

private:
    void CreateDSS(const ComPtr<ID3D11Device>& dev);
    void CreateRS(const ComPtr<ID3D11Device>& dev);
    void CreateSampler(const ComPtr<ID3D11Device>& dev);
    void CreateBS(const ComPtr<ID3D11Device>& dev);
    void CreateInputLayoutShader(const ComPtr<ID3D11Device>& dev, const ComPtr<ID3D11DeviceContext>& ctx);
    void CreateCB(const ComPtr<ID3D11Device>& dev);


public:
    // DSS
    ComPtr<ID3D11DepthStencilState>   defualtDSS;                 // depth test on + write on
    ComPtr<ID3D11DepthStencilState>   depthTestOnlyDSS;           // depth test only
    ComPtr<ID3D11DepthStencilState>   depthTestStencilWriteDSS;   // depth test only / stencil write on (stencil test ALWAYS)
    ComPtr<ID3D11DepthStencilState>   stencilTestOnlyDSS;         // stencil test only
    ComPtr<ID3D11DepthStencilState>   disableDSS;                 // all disable

    // RS
    ComPtr<ID3D11RasterizerState>     cullfrontRS;         // cullmode = front : 앞면 버리고 뒷면만 남김
    ComPtr<ID3D11RasterizerState>     cullNoneRS;          // 컬링 안하고 모든 영역을 그림

    // Sampler
    ComPtr<ID3D11SamplerState>	linearSamplerState;       // linear    
    ComPtr<ID3D11SamplerState>  shadowSamplerState;       // clmap
    ComPtr<ID3D11SamplerState>	linearClamSamplerState;   // linear + clamp    

    // Blend State
    ComPtr<ID3D11BlendState>  alphaBlendState;          // alpha
    ComPtr<ID3D11BlendState>  additiveBlendState;       // additive (multiple light)
    
    // IA
    ComPtr<ID3D11InputLayout> inputLayout_Vertex;
    ComPtr<ID3D11InputLayout> inputLayout_Position;
    ComPtr<ID3D11InputLayout> inputLayout_Particle;

    // Vertex Shader
    ComPtr<ID3D11VertexShader> VS_ShadowDepth_Model;
    ComPtr<ID3D11VertexShader> VS_BaseLit_Model;
    ComPtr<ID3D11VertexShader> VS_Skybox;
    ComPtr<ID3D11VertexShader> VS_FullScreen;
    ComPtr<ID3D11VertexShader> VS_LightVolume;
    ComPtr<ID3D11VertexShader> VS_Effect;

    // Pixel Shader
    ComPtr<ID3D11PixelShader> PS_ShadowDepth;
    ComPtr<ID3D11PixelShader> PS_Gbuffer;
    ComPtr<ID3D11PixelShader> PS_DeferredLighting;
    ComPtr<ID3D11PixelShader> PS_Effect;
    ComPtr<ID3D11PixelShader> PS_Skybox;
    ComPtr<ID3D11PixelShader> PS_BloomPrefilter;
    ComPtr<ID3D11PixelShader> PS_BloomDownsampleBlur;
    ComPtr<ID3D11PixelShader> PS_BloomUpsampleCombine;
    ComPtr<ID3D11PixelShader> PS_PostProcess;


    // Constant Bufer
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