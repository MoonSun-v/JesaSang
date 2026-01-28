#if _DEBUG
#pragma once
#include "pch.h"
#include "System/InputSystem.h"
#include "../Object/GameObject.h"

/// @brief imgui를 사용하고 렌더링 하는 객체
/// @date 26 01 07
/// @details
class Editor : public InputProcesser
{
public:
    void GetScreenSize(int width, int height) { screenWidth = width; screenHeight = height; }
    void Initialize(const ComPtr<ID3D11Device>& device, const ComPtr<ID3D11DeviceContext>& context);
    void GetDSV(const ComPtr<ID3D11DepthStencilView>& dsv) { depthStencliView = dsv; };
    void GetRTV(const ComPtr <ID3D11RenderTargetView>& rtv) { renderTargetView = rtv; };

    void Update();
    void Render(HWND& hwnd);
    void RenderEnd(const ComPtr<ID3D11DeviceContext>& context);

    void SelectObject(GameObject* obj);

    void ReleaseBackBufferResources();
    void CreatePickingStagingTex();

private:
    void RenderMenuBar(HWND& hwnd);
    void RenderHierarchy();
    void DrawHierarchyNode(GameObject* obj);
    void DrawHierarchyDropSpace();
    void RenderInspector();
    void RenderPlayModeControls();
    void RenderCameraFrustum();
    void RenderWorldSettings();
    void RenderShadowMap();

    void RenderWorldManager();

    template<typename T>
    void RenderComponentInfo(std::string name, T* comp);

    void RenderDebugAABBDraw();

    void SaveCurrentScene(HWND& hwnd);
    void LoadScene(HWND& hwnd);
    
    GameObject* selectedObject; // 현재 inspector 정보를 보고 있는 게임 오브젝트

    // 카메라 정보
    Matrix cameraView{};
    Matrix cameraProjection{};

    // 화면 정보
    int screenWidth = 0;
    int screenHeight = 0;

    // 디버그 바인드 
    ComPtr<ID3D11Device>                device{};
    ComPtr<ID3D11DeviceContext>         context{};
    ComPtr<ID3D11InputLayout> 			inputLayout{};
    ComPtr<ID3D11RenderTargetView> 		renderTargetView{};
    ComPtr<ID3D11DepthStencilView>		depthStencliView{};
    ComPtr<ID3D11RasterizerState>       rasterizerState{};

    // flags
    bool isDiretionalLightDebugOpen = false;
    bool isWorldSettingOpen = false;
    bool isPhysicsDebugOpen = false;

    std::string currScenePath{};

    // object picking 
    bool isMouseLeftClick = false;
    XMINT2 mouseXY{};
    uint32_t currPickedID = -1;
    ComPtr<ID3D11Texture2D> coppedPickingTex{};

    void CheckObjectPicking();
    bool isAABBPicking = false; // 매 프레임마다 초기화 ( aabb와 메쉬 구분 )

    // rttr read
    void ReadVariants(rttr::variant& var);
    void ReadVariants(rttr::instance inst);

public:
	void OnInputProcess(const Keyboard::State& KeyState, const Keyboard::KeyboardStateTracker& KeyTracker,
		const Mouse::State& MouseState, const Mouse::ButtonStateTracker& MouseTracker) override;
};
#endif