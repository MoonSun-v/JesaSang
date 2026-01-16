#if _DEBUG
#pragma once
#include "../pch.h"
#include "wrl/client.h"
#include "d3d11.h"

using namespace Microsoft::WRL;

class ImguiRenderer
{
private:
    std::vector<std::function<void()>> renderContents; // Inspector, Hierarchy, SceneView ...

public:
    ImguiRenderer() = default;
    ~ImguiRenderer();

    void Initialize(HWND hwnd, ComPtr<ID3D11Device> device, ComPtr<ID3D11DeviceContext> deviceContex);
    void BeginRender();
    void Render();
    void EndRender();

    void AddRenderContents(std::function<void()> contants);
};
#endif