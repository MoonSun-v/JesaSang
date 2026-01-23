#if _DEBUG
#include "Editor.h"
#include "../EngineSystem/CameraSystem.h"
#include "../Manager/ComponentFactory.h"
#include "../EngineSystem/SceneSystem.h"

#include <commdlg.h>
#include "imguiFileDialog/ImGuiFileDialog.h"
#include "../Components/FBXData.h"
#include "../Object/GameObject.h"
#include "../Util/DebugDraw.h"
#include "../Manager/WorldManager.h"
#include "../Manager/Shadermanager.h"
#include "../EngineSystem/PlayModeSystem.h"
#include "../Components/Camera.h"

#include "Datas/ReflectionMedtaDatas.hpp"

// 사용자 정의 미리 등록 (SimpleMath 등)
RTTR_REGISTRATION
{
    using namespace rttr;

    registration::class_<DirectX::SimpleMath::Vector2>("Vector2")
        .constructor<>()
        .constructor<float, float>()
        .property("x", &Vector2::x)
        .property("y", &Vector2::y);

    registration::class_<DirectX::SimpleMath::Vector3>("Vector3")
        .constructor<>()
        .constructor<float, float, float>()
        .property("x", &Vector3::x)
        .property("y", &Vector3::y)
        .property("z", &Vector3::z);

    registration::class_<DirectX::SimpleMath::Vector4>("Vector4")
        .constructor<>()
        .constructor<float, float, float, float>()
        .property("x", &Vector4::x)
        .property("y", &Vector4::y)
        .property("z", &Vector4::z)
        .property("w", &Vector4::w);

    registration::class_<DirectX::SimpleMath::Quaternion>("Quaternion")
        .constructor<>()
        .constructor<float, float, float, float>()
        .property("x", &Quaternion::x)
        .property("y", &Quaternion::y)
        .property("z", &Quaternion::z)
        .property("w", &Quaternion::w);

    registration::class_<DirectX::SimpleMath::Matrix>("Matrix")
        .constructor<>()
        .property("_11", &Matrix::_11)
        .property("_12", &Matrix::_12)
        .property("_13", &Matrix::_13)
        .property("_14", &Matrix::_14)
        .property("_21", &Matrix::_21)
        .property("_22", &Matrix::_22)
        .property("_23", &Matrix::_23)
        .property("_24", &Matrix::_24)
        .property("_31", &Matrix::_31)
        .property("_32", &Matrix::_32)
        .property("_33", &Matrix::_33)
        .property("_34", &Matrix::_34)
        .property("_41", &Matrix::_41)
        .property("_42", &Matrix::_42)
        .property("_43", &Matrix::_43)
        .property("_44", &Matrix::_44);
}

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
    RenderCameraFrustum();
    RenderWorldSettings();
    RenderShadowMap();
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

void Editor::ReleaseBackBufferResources()
{
    depthStencliView.Reset();
    renderTargetView.Reset();
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
        if (ImGui::BeginMenu("World Setting"))
        {
            if (ImGui::MenuItem("World Setting"))
            {
                isWorldSettingOpen = !isWorldSettingOpen;
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
                        if (name == "Transform") continue; // transform은 무시 ( 왜나면 Transform은 한 게임 오브젝트에 한 개만 존재한다. )

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
                        ImGui::Separator();
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

void Editor::RenderCameraFrustum()
{
    auto cams = CameraSystem::Instance().GetAllCamera();

    DebugDraw::g_Batch->Begin();
    for (auto& cam : cams)
    {
        if (cam->GetOwner()->GetName() == "FreeCamera") continue;

        DirectX::BoundingFrustum frustum;
        DirectX::BoundingFrustum::CreateFromMatrix(
            frustum,
            cam->GetProjection()
        );

        Matrix frustumWorld = cam->GetView().Transpose();
        frustum.Transform(frustum, frustumWorld);
        
        Matrix camWorld = cam->GetOwner()->GetTransform()->GetWorldTransform();
        frustum.Transform(frustum, camWorld);

        DebugDraw::Draw(DebugDraw::g_Batch.get(), frustum);
    }

    DebugDraw::g_Batch->End();
}

void Editor::RenderWorldSettings()
{
    if (isWorldSettingOpen)
    {
        RenderWorldManager();
    }
}

void Editor::RenderShadowMap()
{
    if (isDiretionalLightDebugOpen)
    {
        ImGui::Begin("Shadow Map");

        ID3D11ShaderResourceView* shadowSRV = ShaderManager::Instance().shadowSRV.Get();
        ImVec2 size(256, 256);

        ImGui::Image(
            (ImTextureID)shadowSRV,
            size,
            ImVec2(0, 1),   
            ImVec2(1, 0)    
        );

        ImGui::End();
    }
}

void Editor::RenderWorldManager()
{
    // Read
    WorldManager& wm = WorldManager::Instance(); // 또는 Instance() 등 프로젝트 방식대로

    rttr::instance inst = wm;
    rttr::type t = rttr::type::get(inst);

    // 1. worldManager의 인스턴스를 렌더링한다. ( 구조체, 클래스 내용 제외 )
    ReadVariants(inst); 

    ImGui::Separator();
    // 2. shadow data
    ReadVariants(wm.shadowData);

    ImGui::Separator();
    // 3. postProcess data
    ReadVariants(wm.postProcessData);

    ImGui::Separator();
    // 4. FrameWorld data
    ReadVariants(wm.frameData);
}

template<typename T>
void Editor::RenderComponentInfo(std::string compName, T* comp)
{
    rttr::type t = rttr::type::get(*comp); // 역참조로 실제 인스턴스 정보 가져오기
    ImGui::Text(t.get_name().to_string().c_str());
    if(compName == "Transform")
    {
        for(auto& prop : t.get_properties())
        {
            rttr::variant value = prop.get_value(*comp);   // 프로퍼티 값
            std::string name = prop.get_name().to_string();         // 프로퍼티 이름

            if(value.is_type<DirectX::SimpleMath::Vector3>() && name == "Rotation")
            {
                DirectX::SimpleMath::Vector3 rot = value.get_value<DirectX::SimpleMath::Vector3>();
                DirectX::SimpleMath::Vector3 eulerDegree = { XMConvertToDegrees(rot.x), XMConvertToDegrees(rot.y),  XMConvertToDegrees(rot.z) };
                ImGui::DragFloat3("Rotation", &eulerDegree.x, 0.1f);
                rot = { XMConvertToRadians(eulerDegree.x), XMConvertToRadians(eulerDegree.y),  XMConvertToRadians(eulerDegree.z) };
                prop.set_value(*comp, rot);
            }
            else if (value.is_type<DirectX::SimpleMath::Vector3>())
            {
                DirectX::SimpleMath::Vector3 vec = value.get_value<DirectX::SimpleMath::Vector3>();
                ImGui::DragFloat3(name.c_str(), &vec.x, 0.1f);
                prop.set_value(*comp, vec);
            }
        } 
    }
    else if(compName == "FBXData")
    {
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

                        std::filesystem::path relativePath = std::filesystem::relative(filePathName);
                        std::string relativePathStr = relativePath.string();
                        // action

                        FBXData* fbxDataComp = dynamic_cast<FBXData*>(comp);
                        fbxDataComp->ChangeData(relativePathStr);
                    }
                    // close
                    ImGuiFileDialog::Instance()->Close();
                }
            }
        }        
    }
    else
    {
        ImGui::PushID(comp);
        ReadVariants(*comp);
        ImGui::PopID();
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
    
    Camera* cam{}; 
    if (PlayModeSystem::Instance().IsPlaying())
    {
        cam = CameraSystem::Instance().GetCurrCamera();
    }
    else
    {
        cam = CameraSystem::Instance().GetFreeCamera();
    }
    DebugDraw::g_BatchEffect->SetView(cam->GetView());
    DebugDraw::g_BatchEffect->SetProjection(cam->GetProjection());

    DebugDraw::g_BatchEffect->SetWorld(Matrix::Identity);
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

void Editor::ReadVariants(rttr::variant& var)
{
    ReadVariants(rttr::instance(var));
}

void Editor::ReadVariants(rttr::instance inst)
{
    if (!inst.is_valid())
        return;

    rttr::type t = inst.get_derived_type();


    // Get value from type
    for (auto& prop : t.get_properties())
    {
        rttr::variant value = prop.get_value(inst);
        std::string name = prop.get_name().to_string();

        if (!value.is_valid())
            continue;

        // check metaData
        auto metaBool = prop.get_metadata(META_BOOL);

        // Render elements
        // ImGui::Text("%s : %s", name.c_str(), value.get_type().get_name().to_string().c_str());

        if (metaBool.is_valid() && metaBool.to_bool())
        {
            int iv = value.to_int();     // BOOL이든 int든 흡수
            bool b = (iv != 0);
            if (ImGui::Checkbox(name.c_str(), &b))
            {
                prop.set_value(inst, b ? 1 : 0);
            }
        }
        else if (value.is_type<float>())
        {
            float v = value.get_value<float>();
            if (ImGui::DragFloat(name.c_str(), &v, 0.1f))
                prop.set_value(inst, v);
        }
        else if (value.is_type<int>())
        {
            int v = value.get_value<int>();
            if (ImGui::DragInt(name.c_str(), &v, 1))
                prop.set_value(inst, v);
        }
        else if (value.is_type<bool>())
        {
            bool v = value.get_value<bool>();
            if (ImGui::Checkbox(name.c_str(), &v))
                prop.set_value(inst, v);
        }
        else if (value.is_type<DirectX::SimpleMath::Vector3>())
        {
            auto vec = value.get_value<DirectX::SimpleMath::Vector3>();
            if (ImGui::DragFloat3(name.c_str(), &vec.x, 0.1f))
                prop.set_value(inst, vec);
        }
        else if (value.is_type<Color>())
        {
            auto c = value.get_value<Color>();
            if (ImGui::ColorEdit3(name.c_str(), &c.x))
                prop.set_value(inst, c);
        }
    }
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