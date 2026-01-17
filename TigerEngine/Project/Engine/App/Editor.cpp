#if _DEBUG
#include "Editor.h"
#include <System/CameraSystem.h>
#include <System/ComponentFactory.h>
#include "System/SceneSystem.h"

#include <commdlg.h>
#include "imguiFileDialog/ImGuiFileDialog.h"
#include "../Components/FBXData.h"
#include "Entity/Camera.h"
#include "../Util/DebugDraw.h"
#include "../Manager/WorldManager.h"
#include "System/PlayModeSystem.h"

void Editor::Initialize(const ComPtr<ID3D11Device>& device, const ComPtr<ID3D11DeviceContext>& deviceContext)
{
    DebugDraw::Initialize(device, deviceContext);
    this->device = device;
    this->context = deviceContext;
}

void Editor::Update()
{
    Scene* currScene = SceneSystem::Instance().GetCurrentScene().get();

    currScene->ForEachGameObject([](GameObject* obj){
        if(obj->GetName() == "FreeCamera") return;        
        obj->UpdateAABB();
     });    
}

void Editor::Render(HWND &hwnd)
{
    RenderMenuBar(hwnd);
    RenderHierarchy();
    RenderInspector();
    RenderDebugAABBDraw();
    DirectionalLightDebug();
}

void Editor::RenderEnd(const ComPtr<ID3D11DeviceContext>& context)
{
    context->OMSetBlendState(nullptr, nullptr, 0xFFFFFFFF);
    context->OMSetDepthStencilState(nullptr, 0);
    context->RSSetState(nullptr);
}

void Editor::SelectObject(GameObject* obj)
{
    selectedObject = obj;
}

void Editor::RenderMenuBar(HWND& hwnd)
{
    if (ImGui::BeginMainMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {
            if (ImGui::MenuItem("Save current scene"))
			{
				SaveCurrentScene(hwnd);
			}
            else if(ImGui::MenuItem("Load scene"))
            {
                LoadScene(hwnd);
            }

            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Debug"))
        {
            if (ImGui::MenuItem("Directional Shadow"))
            {
                isDiretionalLightDebugOpen = !isDiretionalLightDebugOpen;
            }
            ImGui::EndMenu();
        }

        RenderPlayModeControls();
    }
    ImGui::EndMainMenuBar();
}

void Editor::RenderHierarchy()
{
    ImGui::Begin("World Hierarchy");
    {
        if(ImGui::Button("Create GameObject"))
        {
            SceneSystem::Instance().GetCurrentScene()->AddGameObjectByName("NewGameObject");
        }

        SceneSystem::Instance().GetCurrentScene()->ForEachGameObject([this](GameObject* obj)
        {
            ImGui::PushID(obj); // 고유 ID 부여 (ID 충돌 방지)
            
            if (ImGui::Selectable(obj->GetName().c_str(), selectedObject == obj))
            {
                SelectObject(obj);
            }

            ImGui::PopID();
        });
    }
    ImGui::End();
}

void Editor::RenderInspector()
{
    ImGui::Begin("Inspector");
    {
        if(selectedObject == nullptr)
        {
            ImGui::Text("No gameObject selected");
        }
        else
        {
            auto obj = selectedObject;
            if (!obj->IsDestory())
            {
                /* ------------------------------- gameobject ------------------------------- */
                rttr::type t = rttr::type::get(obj);
                ImGui::Text("Type : %s", t.get_name().to_string().c_str());

                for (auto& prop : t.get_properties())
                {
                    rttr::variant value = prop.get_value(obj);   // 프로퍼티 값
                    std::string name = prop.get_name().to_string();         // 프로퍼티 이름
                    if (value.is_type<std::string>() && name == "Name")
                    {
                        ImGui::Text("Name : %s", name.c_str());
                        char buf[256]{};
                        strncpy_s(buf, value.get_value<std::string>().c_str(), sizeof(buf) - 1);
                        ImGui::InputText(name.c_str(), buf, sizeof(buf), ImGuiInputTextFlags_EnterReturnsTrue);
                        prop.set_value(obj, std::string(buf));
                    }
                }

                /* -------------------------------- transform ------------------------------- */
                if (ImGui::Button("Destory"))
                {
                    selectedObject = nullptr;
                    obj->Destory();
                }

                /* ---------------------------- add component 내용 ---------------------------- */
                if (ImGui::Button("Add Component"))
                {
                    ImGui::OpenPopup("ComponentMenu"); // 1. popup 열라고 명령 
                    // open component menu
                    // - select component -> ???
                    // - call obj->AddComponent<T>()
                }

                // 2. 해당 ID를 가진 팝업이 열려있는지 확인하고 그림
                if (ImGui::BeginPopup("ComponentMenu"))
                {
                    auto& componentsMap = ComponentFactory::Instance().GetRegisteredComponents();

                    for (auto& [name, creatorFunc] : componentsMap)
                    {
                        // 컴포넌트 이름을 버튼 (MenuItem)으로 노출
                        if (ImGui::MenuItem(name.c_str()))
                        {
                            // 1. 생성 람다 함수를 통해 새 컴포넌트 생성
                            creatorFunc(obj);

                            // 2. 현재 작업 중인 오브젝트에 추가
                            // GameObject에 AddComponent(std::shared_ptr<Component>) 형태의 함수가 있어야 합니다.
                            // obj->AddComponent(newComp); 
                            ImGui::CloseCurrentPopup();
                        }
                    }

                    ImGui::Separator();
                    if (ImGui::MenuItem("Close")) { ImGui::CloseCurrentPopup(); }

                    ImGui::EndPopup();
                }

                /* ------------------------------- 컴포넌트 내용 출력 ------------------------------- */
                for (auto& comp : obj->GetComponents())
                {
                    auto registeredComps = ComponentFactory::Instance().GetRegisteredComponents();
                    auto name = comp->GetName();
                    if (auto it = registeredComps.find(name); it != registeredComps.end())
                    {
                        RenderComponentInfo(it->first, comp);
                        ImGui::NewLine();
                    }
                }
            }
        }
    }
    ImGui::End();
}

void Editor::RenderPlayModeControls()
{
    auto& playMode = PlayModeSystem::Instance();
    PlayModeState currentState = playMode.GetPlayMode();

    // 현재 상태에 따라 버튼 색상 설정
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.7f, 0.2f, 1.0f)); // Play - 초록색
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.8f, 0.3f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.1f, 0.6f, 0.1f, 1.0f));

    // Play 버튼
    if (ImGui::Button("Play"))
    {
        playMode.SetPlayMode(PlayModeState::Playing);
    }
    ImGui::PopStyleColor(3);

    ImGui::SameLine();

    // Pause 버튼 - 노란색
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.7f, 0.7f, 0.2f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.8f, 0.8f, 0.3f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.6f, 0.6f, 0.1f, 1.0f));

    if (ImGui::Button("Pause"))
    {
        if (currentState == PlayModeState::Playing)
        {
            playMode.SetPlayMode(PlayModeState::Paused);
        }
        else if (currentState == PlayModeState::Paused)
        {
            playMode.SetPlayMode(PlayModeState::Playing);
        }
    }
    ImGui::PopStyleColor(3);

    ImGui::SameLine();

    // Stop 버튼 - 빨간색
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.7f, 0.2f, 0.2f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.8f, 0.3f, 0.3f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.6f, 0.1f, 0.1f, 1.0f));

    if (ImGui::Button("Stop"))
    {
        playMode.SetPlayMode(PlayModeState::Stopped);
    }
    ImGui::PopStyleColor(3);

    ImGui::SameLine();
    ImGui::Separator();
    ImGui::SameLine();

    // 현재 상태 표시
    const char* stateText = "";
    ImVec4 stateColor;
    switch (currentState)
    {
    case PlayModeState::Stopped:
        stateText = "Stopped";
        stateColor = ImVec4(0.7f, 0.7f, 0.7f, 1.0f); // 회색
        break;
    case PlayModeState::Playing:
        stateText = "Playing";
        stateColor = ImVec4(0.2f, 0.7f, 0.2f, 1.0f); // 초록색
        break;
    case PlayModeState::Paused:
        stateText = "Paused";
        stateColor = ImVec4(0.7f, 0.7f, 0.2f, 1.0f); // 노란색
        break;
    }

    ImGui::PushStyleColor(ImGuiCol_Text, stateColor);
    ImGui::Text(" [%s]", stateText);
    ImGui::PopStyleColor();
}

template<typename T>
void Editor::RenderComponentInfo(std::string compName, T* comp)
{
    if(compName == "Transform")
    {
        rttr::type t = rttr::type::get(*comp); // 역참조로 실제 인스턴스 정보 가져오기
        ImGui::Text(t.get_name().to_string().c_str());

        for(auto& prop : t.get_properties())
        {
            rttr::variant value = prop.get_value(*comp);   // 프로퍼티 값
            std::string name = prop.get_name().to_string();         // 프로퍼티 이름
            if(value.is_type<DirectX::SimpleMath::Vector3>() && name == "Position")
            {
                DirectX::SimpleMath::Vector3 pos = value.get_value<DirectX::SimpleMath::Vector3>();
                ImGui::DragFloat3("Position", &pos.x, 0.1f);
                prop.set_value(*comp, pos);
            }
            else if(value.is_type<DirectX::SimpleMath::Vector3>() && name == "Rotation")
            {
                DirectX::SimpleMath::Vector3 rot = value.get_value<DirectX::SimpleMath::Vector3>();
                DirectX::SimpleMath::Vector3 rotEuler = { XMConvertToDegrees(rot.x), XMConvertToDegrees(rot.y),  XMConvertToDegrees(rot.z) };
                ImGui::DragFloat3("Rotation", &rotEuler.x, 0.1f);
                rot = { XMConvertToRadians(rotEuler.x), XMConvertToRadians(rotEuler.y),  XMConvertToRadians(rotEuler.z) };
                prop.set_value(*comp, rot);
            }
            else if(value.is_type<DirectX::SimpleMath::Vector3>() && name == "Scale")
            {
                DirectX::SimpleMath::Vector3 scl = value.get_value<DirectX::SimpleMath::Vector3>();
                ImGui::DragFloat3("Scale", &scl.x, 0.1f);
                prop.set_value(*comp, scl);
            }
        } 
    }
    else if(compName == "FBXData")
    {
        rttr::type t = rttr::type::get(*comp); // 역참조로 실제 인스턴스 정보 가져오기
        ImGui::Text(t.get_name().to_string().c_str());

        for(auto& prop : t.get_properties())
        {
            rttr::variant value = prop.get_value(*comp);   // 프로퍼티 값
            std::string name = prop.get_name().to_string();// 프로퍼티 이름
            if(value.is_type<std::string>() && name == "DataPath")
            {
                std::string path = value.get_value<std::string>();

                // 현재 경로 표시   
                ImGui::Text("Current Path: %s", path.c_str());
                
                // 탐색기 열기 버튼
                if (ImGui::Button("Browse..."))
                {
                    IGFD::FileDialogConfig config;
                    config.path = "../";
                    ImGuiFileDialog::Instance()->OpenDialog("ChooseFileDlgKey", "Choose File", ".fbx", config);
                }
                    // display
                if (ImGuiFileDialog::Instance()->Display("ChooseFileDlgKey")) 
                {
                    if (ImGuiFileDialog::Instance()->IsOk()) 
                    { // action if OK
                        std::string filePathName = ImGuiFileDialog::Instance()->GetFilePathName();     // 절대 경로 + 파일 이름
                        std::string currFilePath = ImGuiFileDialog::Instance()->GetCurrentFileName();   // 진짜 파일 이름만 뜸
                        std::string filePath = ImGuiFileDialog::Instance()->GetCurrentPath();           // 절대 경로만 뜸
                        std::string fileFilterPath = ImGuiFileDialog::Instance()->GetCurrentFilter();   // 확장자만 나옴
                        // action

                        FBXData* fbxDataComp = dynamic_cast<FBXData*>(comp);
                        fbxDataComp->ChangeData(filePathName);
                    }
                    // close
                    ImGuiFileDialog::Instance()->Close();
                }
            }
        }        
    }
    else if(compName == "FBXRenderer")
    {
        rttr::type t = rttr::type::get(*comp);
        ImGui::Text(t.get_name().to_string().c_str());

        for(auto& prop : t.get_properties())
        {
            rttr::variant value = prop.get_value(*comp);            // 프로퍼티 값
            std::string name = prop.get_name().to_string();         // 프로퍼티 이름
            if (value.is_type<Color>() && name == "Color")
            {
                Color v = value.get_value<Color>();
                ImGui::ColorEdit3("Color", &v.x);
                prop.set_value(*comp, v);
            }
            else if (value.is_type<float>() && name == "Roughness")
            {
                float v = value.get_value<float>();
                ImGui::DragFloat("Roughness", &v, 0.1f, 0.0f, 1.0f);
                prop.set_value(*comp, v);
            }
            else if (value.is_type<float>() && name == "Metalic")
            {
                float v = value.get_value<float>();
                ImGui::DragFloat("Metalic", &v, 0.1f, 0.0f, 1.0f);
                prop.set_value(*comp, v);
            }
            else if(value.is_type<int>() && name == "AnimationIndex")
            {
                int v = value.get_value<int>();
                ImGui::InputInt("Play Animation Index", &v);
                // prop.set_value(*comp, v);
            }
            else if(value.is_type<float>() && name == "AnimationPlayTime")
            {
                float v = value.get_value<float>();
                ImGui::DragFloat("Time", &v, 0.1f);
                prop.set_value(*comp, v);
            }
            else if(value.is_type<bool>() && name == "IsAnimationPlay")
            {
                bool v = value.get_value<bool>();
                ImGui::Checkbox("isPlay", &v);
                prop.set_value(*comp, v);
            }
        } 
    }
    else // TODO : Value 테스트
    {
        rttr::type t = rttr::type::get(*comp); // 역참조로 실제 인스턴스 정보 가져오기
        ImGui::Text(t.get_name().to_string().c_str());
        for (auto& prop : t.get_properties())
        {
            rttr::variant value = prop.get_value(*comp);            // 프로퍼티 값
            std::string name = prop.get_name().to_string();         // 프로퍼티 이름

            if (value.is_type<float>())
            {
                float v = value.get_value<float>();
                ImGui::DragFloat(name.c_str(), &v, 0.1f);
                prop.set_value(*comp, v);
            }
        }
    }

    if (compName != "Transform") 
    {
        ImGui::PushID(comp);
        if(ImGui::Button("Remove Component"))
        {
            selectedObject->RemoveComponent(comp);
        }
        ImGui::PopID();
    }
}

void Editor::RenderDebugAABBDraw()
{
    // 렌더타겟 다시 설정 (ImGui가 변경했을 수 있음)
    context->OMSetRenderTargets(1, renderTargetView.GetAddressOf(), depthStencliView.Get());

    // DebugDraw의 BasicEffect 설정
    auto cam = CameraSystem::Instance().GetFreeCamera();
    DebugDraw::g_BatchEffect->SetWorld(Matrix::Identity);
    DebugDraw::g_BatchEffect->SetView(cam->GetView());
    DebugDraw::g_BatchEffect->SetProjection(cam->GetProjection());
    DebugDraw::g_BatchEffect->Apply(context.Get());

    // InputLayout 설정
    context->IASetInputLayout(DebugDraw::g_pBatchInputLayout.Get());

    // 블렌드 스테이트 설정 (깊이 테스트 활성화)
    context->OMSetBlendState(DebugDraw::g_States->AlphaBlend(), nullptr, 0xFFFFFFFF);
    context->OMSetDepthStencilState(DebugDraw::g_States->DepthRead(), 0);
    context->RSSetState(DebugDraw::g_States->CullNone());


    // 선택된 오브젝트는 밝은 초록색
    SceneSystem::Instance().GetCurrentScene()->ForEachGameObject([&](GameObject* gameObject) {
        if (gameObject->IsDestory()) return;

        XMVECTOR color = XMVectorSet(0.0f, 1.0f, 0.0f, 1.0f);
        DebugDraw::g_Batch->Begin();
        DebugDraw::Draw(DebugDraw::g_Batch.get(), gameObject->GetAABB(), color);
        DebugDraw::g_Batch->End();
     });
}

void Editor::SaveCurrentScene(HWND& hwnd)
{
	// 파일 저장 다이얼로그
	OPENFILENAMEA ofn = {};
	char szFile[260] = {};

	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = hwnd;
	ofn.lpstrFile = szFile;
	ofn.nMaxFile = sizeof(szFile);
	ofn.lpstrFilter = "JSON Files (*.json)\0*.json\0All Files (*.*)\0*.*\0";
	ofn.nFilterIndex = 1;
	ofn.lpstrFileTitle = NULL;
	ofn.nMaxFileTitle = 0;
	ofn.lpstrInitialDir = NULL;
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT 
            | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;;
	ofn.lpstrDefExt = "json";

	if (GetSaveFileNameA(&ofn) != TRUE)
		return; // 사용자가 취소함

	std::string filename = szFile;

	// GameWorld를 파일에 저장
	if (SceneSystem::Instance().GetCurrentScene()->SaveToJson(filename))
	{
		MessageBoxA(hwnd, "Scene saved successfully!", "Save", MB_OK | MB_ICONINFORMATION);        
	}
	else
	{
		MessageBoxA(hwnd, "Failed to save scene!", "Error", MB_OK | MB_ICONERROR);
	}
}

void Editor::LoadScene(HWND &hwnd)
{
    OPENFILENAMEA ofn ={};
    char szFile[256] = {};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hwnd;
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = sizeof(szFile);
	ofn.lpstrFilter = "JSON Files (*.json)\0*.json\0All Files (*.*)\0*.*\0";
	ofn.nFilterIndex = 1;
	ofn.lpstrFileTitle = NULL;
	ofn.nMaxFileTitle = 0;
	ofn.lpstrInitialDir = NULL;
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT 
            | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;
	ofn.lpstrDefExt = "json";

    // NOTE : GetOpenFileNameA를 한 뒤로 CWD (Current Working Directory)가 선택한 폴더로 변경된다.
    // ->  OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR 플래그 추가 해줘서 방지
    if (GetOpenFileNameA(&ofn) != TRUE) 
	    return; // 사용자가 취소함

    std::string filename = szFile;

    auto scene = SceneSystem::Instance().GetCurrentScene();

    // scene으로 파일 데이터 로드하기
    if (scene->LoadToJson(filename))
    {
    	MessageBoxA(hwnd, "Scene loaded successfully!", "Load", MB_OK | MB_ICONINFORMATION);
    }
    else
    {
    	MessageBoxA(hwnd, "Failed to load scene! File not found.", "Error", MB_OK | MB_ICONERROR);
    }
}

void Editor::DirectionalLightDebug()
{
    if (!isDiretionalLightDebugOpen) return;

    ImGui::Begin("ShadowMap");
    {
        ImTextureID img = (ImTextureID)(intptr_t)(WorldManager::Instance().shaderResourceView.Get());
        ImGui::Image(img, ImVec2(256, 256));


        // Direction
        auto& world = WorldManager::Instance();

        // Direction
        ImGui::Text("Light Direction");
        ImGui::SliderFloat3(
            "Direction",
            &world.lightDirection.x,
            -1.0f,
            1.0f
        );
        if (ImGui::Button("Normalize Direction"))
        {
            XMVECTOR dir = XMVectorSet(
                world.lightDirection.x,
                world.lightDirection.y,
                world.lightDirection.z,
                0.0f
            );
            dir = XMVector3Normalize(dir);
            XMStoreFloat4(&world.lightDirection, dir);
        }
        ImGui::Separator();
        // LookAt / Position
        ImGui::Text("Light Transform");
        ImGui::DragFloat3(
            "LookAt",
            &world.directionalLightLookAt.x,
            1.0f
        );
        ImGui::DragFloat3(
            "Position",
            &world.directionalLightPos.x,
            1.0f
        );
        ImGui::DragFloat3(
            "Up Offset",
            &world.directionalLightUpDistFromLookAt.x,
            1.0f,
            -5000.0f,
            5000.0f
        );
        ImGui::Separator();
        // Frustum
        ImGui::Text("Frustum");
        ImGui::SliderAngle(
            "Frustum Angle",
            &world.directionalLightFrustumAngle,
            1.0f,
            120.0f
        );
        ImGui::DragFloat(
            "Forward Dist From Camera",
            &world.directionalLightForwardDistFromCamera,
            1.0f,
            0.0f,
            5000.0f
        );
        ImGui::DragFloat(
            "Near Plane",
            &world.directionalLightNear,
            1.0f,
            1.0f,
            world.directionalLightFar - 1.0f
        );
        ImGui::DragFloat(
            "Far Plane",
            &world.directionalLightFar,
            10.0f,
            world.directionalLightNear + 1.0f,
            20000.0f
        );
        ImGui::Separator();
        // Shadow Map Viewport
        ImGui::Text("Shadow Map Viewport");
        ImGui::DragFloat2(
            "Size",
            &world.directionalLightViewport.width,
            1.0f,
            256.0f,
            16384.0f
        );
    }
    ImGui::End();
}

void Editor::OnInputProcess(const Keyboard::State &KeyState, const Keyboard::KeyboardStateTracker &KeyTracker, const Mouse::State &MouseState, const Mouse::ButtonStateTracker &MouseTracker)
{
    // check picking gameOject
    
    if(MouseTracker.leftButton == Mouse::ButtonStateTracker::PRESSED)
    {
        if(!ImGui::GetIO().WantCaptureMouse)
        {
            // 마우스 스크린 좌표를 [0, 1] -> [-1, 1] 로 변경
            float x = (2.0f * MouseState.x) / screenWidth - 1.0f;
            float y = 1.0f - (2.0f * MouseState.y) / screenHeight;

            auto cam = CameraSystem::Instance().GetFreeCamera();
            cameraView = cam->GetView();
            cameraProjection = cam->GetProjection();
            Matrix invViewProj = (cameraView * cameraProjection).Invert();

            Vector4 nearNDC(x, y, 0.0f, 1.0f);
            Vector4 farNDC(x, y, 1.0f, 1.0f);

            // NDC -> World
            Vector4 nearWorld = Vector4::Transform(nearNDC, invViewProj);
            Vector4 farWorld = Vector4::Transform(farNDC, invViewProj);
            
            // 투영 행렬은 원근을 만들기 때문에 perpective divide로 월드 좌표를 얻는다.
            nearWorld /= nearWorld.w;
            farWorld /= farWorld.w;

            Vector3 dir = (Vector3)farWorld - nearWorld;

            dir.Normalize();
            Ray ray(Vector3(nearWorld), dir);

            float outHitDistance = 0.0f;
            auto hitObject = SceneSystem::Instance().GetCurrentScene()->RayCastGameObject(ray, &outHitDistance);

            SelectObject(hitObject);
        }
    }
}
#endif