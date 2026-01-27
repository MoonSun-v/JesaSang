#pragma once
#include "../../Base/pch.h"
#include "../../Base/System/Singleton.h"
#include "../../Base/Datas/ConstantBuffer.hpp"

/// @brief 셰이더에 사용하는 데이터를 가지고 있는 클래스
/// @date 26.01.20
class ShaderManager : public Singleton<ShaderManager>
{
public:
    ShaderManager(token) {};
    ~ShaderManager() = default;

    void Init(const ComPtr<ID3D11Device>& dev, const ComPtr<ID3D11DeviceContext>& ctx
             , float width, float heig);

private:
    void CreateDSS(const ComPtr<ID3D11Device>& dev);
    void CreateRS(const ComPtr<ID3D11Device>& dev);
    void CreateSampler(const ComPtr<ID3D11Device>& dev);
    void CreateBS(const ComPtr<ID3D11Device>& dev);
    void CreateShadowResource(const ComPtr<ID3D11Device>& dev);
    void CreateHDRResource(const ComPtr<ID3D11Device>& dev, int width, int height);
    void CreateGbufferResource(const ComPtr<ID3D11Device>& dev, int screenWidth, int screenHeight);
    void CreateBloomResource(const ComPtr<ID3D11Device>& dev, int screenWidth, int screenHeight);
    void CreateInputLayoutShader(const ComPtr<ID3D11Device>& dev, const ComPtr<ID3D11DeviceContext>& ctx);
    void CreateCB(const ComPtr<ID3D11Device>& dev);


public:
    void CreateBackBufferResource(const ComPtr<ID3D11Device>& dev, int screenWidth, int screenHeight);

    // DX11 Base
    D3D11_VIEWPORT viewport_screen;                                  
    ComPtr<ID3D11RenderTargetView>    backBufferRTV;                 
    ComPtr<ID3D11DepthStencilView>    depthStencilView;          // [stencil buffer] bit0: ground decal mask / bit1: light volume mask  
    ComPtr<ID3D11DepthStencilView>    depthStencilReadOnlyView;      
    ComPtr<ID3D11ShaderResourceView>  depthSRV;                      

    // DSS
    ComPtr<ID3D11DepthStencilState>   defualtDSS;                 // depth test on + write on
    ComPtr<ID3D11DepthStencilState>   depthTestOnlyDSS;           // depth test only
    ComPtr<ID3D11DepthStencilState>   groundDrawDSS;              // depth test on + write on / stencil write on (0x01)
    ComPtr<ID3D11DepthStencilState>   groundTestDSS;              // depth test / stencil test (0x01)
    ComPtr<ID3D11DepthStencilState>   lightingVolumeDrawDSS;      // depth test only / stencil write on (0x02)
    ComPtr<ID3D11DepthStencilState>   lightingVolumeTestDSS;      // stencil test only (0x02)
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
    
    // Shadow Resource
    ComPtr<ID3D11Texture2D>           shadowMap;
    ComPtr<ID3D11DepthStencilView>    shadowDSV;
    ComPtr<ID3D11ShaderResourceView>  shadowSRV;            // ShadowMap Texture
    D3D11_VIEWPORT viewport_shadowMap;

    // HDR
    ComPtr<ID3D11Texture2D>           sceneHDRTex;
    ComPtr<ID3D11RenderTargetView>    sceneHDRRTV;          // HDR RTV
    ComPtr<ID3D11ShaderResourceView>  sceneHDRSRV;          // HDR SRV

    // G-buffer
    ComPtr<ID3D11Texture2D>           albedoTex;
    ComPtr<ID3D11Texture2D>           normalTex;
    ComPtr<ID3D11Texture2D>           metalRoughTex;
    ComPtr<ID3D11Texture2D>           emissiveTex;

    ComPtr<ID3D11RenderTargetView>    albedoRTV;
    ComPtr<ID3D11RenderTargetView>    normalRTV;
    ComPtr<ID3D11RenderTargetView>    metalRoughRTV;
    ComPtr<ID3D11RenderTargetView>    emissiveRTV;

    ComPtr<ID3D11ShaderResourceView>  albedoSRV;
    ComPtr<ID3D11ShaderResourceView>  normalSRV;
    ComPtr<ID3D11ShaderResourceView>  metalRoughSRV;
    ComPtr<ID3D11ShaderResourceView>  emissiveSRV;

    // Bloom
    ComPtr<ID3D11ShaderResourceView>  finalBloomSRV;     // 최종 Bloom SRV
    UINT bloomW;
    UINT bloomH;
    UINT bloomMipCount;
    ComPtr<ID3D11Texture2D>           bloomATex;
    ComPtr<ID3D11Texture2D>           bloomBTex;
    ComPtr<ID3D11ShaderResourceView>  bloomASRV;
    ComPtr<ID3D11ShaderResourceView>  bloomBSRV;
    std::vector<ComPtr<ID3D11RenderTargetView>> bloomARTVs;
    std::vector<ComPtr<ID3D11RenderTargetView>> bloomBRTVs;

    ComPtr<ID3D11Texture2D>           accumATex;
    ComPtr<ID3D11Texture2D>           accumBTex;
    ComPtr<ID3D11ShaderResourceView>  accumASRV;
    ComPtr<ID3D11ShaderResourceView>  accumBSRV;
    std::vector<ComPtr<ID3D11RenderTargetView>> accumARTVs;
    std::vector<ComPtr<ID3D11RenderTargetView>> accumBRTVs;

    // IA
    ComPtr<ID3D11InputLayout> inputLayout_RigidVertex;
    ComPtr<ID3D11InputLayout> inputLayout_BoneWeightVertex;
    ComPtr<ID3D11InputLayout> inputLayout_Position;
    ComPtr<ID3D11InputLayout> inputLayout_Particle;

    // Vertex Shader
    ComPtr<ID3D11VertexShader> VS_ShadowDepth_Rigid;
    ComPtr<ID3D11VertexShader> VS_ShadowDepth_Skeletal;
    ComPtr<ID3D11VertexShader> VS_BaseLit_Rigid;
    ComPtr<ID3D11VertexShader> VS_BaseLit_Skeletal;
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
    ComPtr<ID3D11PixelShader> PS_ForwardTransparent;
    ComPtr<ID3D11PixelShader> PS_BloomPrefilter;
    ComPtr<ID3D11PixelShader> PS_BloomDownsampleBlur;
    ComPtr<ID3D11PixelShader> PS_BloomUpsampleCombine;
    ComPtr<ID3D11PixelShader> PS_PostProcess;

    // CB buffer
    ComPtr<ID3D11Buffer> frameCB;
    ComPtr<ID3D11Buffer> transformCB;
    ComPtr<ID3D11Buffer> lightingCB;
    ComPtr<ID3D11Buffer> materialCB;
    ComPtr<ID3D11Buffer> offsetMatrixCB;
    ComPtr<ID3D11Buffer> poseMatrixCB;
    ComPtr<ID3D11Buffer> postProcessCB;
    ComPtr<ID3D11Buffer> bloomCB;
    ComPtr<ID3D11Buffer> effectCB;
    ComPtr<ID3D11Buffer> decalCB;

    // CB Data
    FrameCB         frameCBData;
    TransformCB     transformCBData;
    LightingCB      lightingCBData;
    MaterialCB      materialCBData;
    OffsetMatrixCB  offsetMatrixCBData;
    PoseMatrixCB    poseMatrixCBData;
    PostProcessCB   postProcessCBData;
    BloomCB         bloomCBData;
    EffectCB        effectCBData;
    DecalCB         decalCBData;

    // Debug Picking 
#if _DEBUG
    ComPtr<ID3D11ShaderResourceView>    pickingSRV;
    ComPtr<ID3D11RenderTargetView>      pickingRTV;
    ComPtr<ID3D11Texture2D>             pickingTex;         // ID 기록 텍스처
    ComPtr<ID3D11Buffer>                pickingCB;          // ID 획득용 상수버퍼
    ComPtr<ID3D11PixelShader>           PS_Picking;
    ComPtr<ID3D11Texture2D>             pickingDepthTex;
    ComPtr<ID3D11DepthStencilView>      pickingDSV;

    // NOTE 따로 볼려고 함수 분리
    void CreatePickingGBufferTex(const ComPtr<ID3D11Device>& dev, int screenWidth, int screenHeight);
    void CreatePickingCB(const ComPtr<ID3D11Device>& dev);
    void CreatePickingPS(const ComPtr<ID3D11Device>& dev);
    void CreatePickingDSV(const ComPtr<ID3D11Device>& dev, int screenWidth, int screenHeight);
#endif

public:
    // Util funcs
    // Texture, RTV, SRV Create Utils
    void CreateRTTex_RTV_SRV(const ComPtr<ID3D11Device>& device, int w, int h, DXGI_FORMAT fomat,
        ID3D11Texture2D** outTex, ID3D11RenderTargetView** outRTV, ID3D11ShaderResourceView** outSRV);

    // mip 해상도 구하기 (baseW/baseH는 bloom base 해상도)
    void GetMipSize(UINT baseW, UINT baseH, UINT mip, UINT& outW, UINT& outH);

    // mip 기준 texel size 구하기
    void GetMipTexelSize(UINT baseW, UINT baseH, UINT mip, float& outTx, float& outTy);

    // viewport 설정
    void SetViewport(const ComPtr<ID3D11DeviceContext>& ctx, UINT width, UINT height);

    // bloom mip용 viewport 설정
    void SetViewportForMip(const ComPtr<ID3D11DeviceContext>& ctx, UINT baseW, UINT baseH, UINT mip);

    // backbuffer 관련 객체 초기화
    void ReleaseBackBufferResources();

};