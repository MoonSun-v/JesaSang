#if _DEBUG
#include "Editor.h"
#include "../EngineSystem/CameraSystem.h"
#include "../Manager/ComponentFactory.h"
#include "../EngineSystem/SceneSystem.h"

#include <commdlg.h>
#include "imguiFileDialog/ImGuiFileDialog.h"
#include "../Components/FBXData.h"
#include "../Components/Decal.h"
#include "../Object/GameObject.h"
#include "../Util/DebugDraw.h"
#include "../Manager/WorldManager.h"
#include "../Manager/Shadermanager.h"
#include "../EngineSystem/PlayModeSystem.h"
#include "../Components/Camera.h"
#include "../EngineSystem/PhysicsSystem.h"
#include "../Components/CharacterControllerComponent.h"

#include "Datas/ReflectionMedtaDatas.hpp"

#include "../Components/FBXRenderer.h"
#include "../Util/PathHelper.h"

// Payload
// Prefab payload
static const char* kPayload_Prefab = "DND_PREFAB";
// 오브젝트 이동 드랍을 위한 payload
static const char* kPayload_GameObject = "DND_GAMEOBJECT";

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

void Editor::GetScreenSize(int width, int height)
{
    screenWidth = width;
    screenHeight = height;
}

void Editor::Initialize(const ComPtr<ID3D11Device>& device, const ComPtr<ID3D11DeviceContext>& deviceContext)
{
    DebugDraw::Initialize(device, deviceContext);

    this->device = device;
    this->context = deviceContext;

    CreatePickingStagingTex();
}

void Editor::Update()
{
    Scene* currScene = SceneSystem::Instance().GetCurrentScene().get();

    currScene->ForEachGameObject([](GameObject* obj){
        if(obj->GetName() == "FreeCamera") return;        
        obj->UpdateAABB();
     });    

    CheckObjectPicking();
    CheckObjectDeleteKey();
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
    RenderPrefabWindow(hwnd);

    ImGui::Begin("DebugPickItem");
    {
        ImGui::Text("%d", currPickedID);
    }
    ImGui::End();
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
            if (ImGui::MenuItem("Physics Collider"))
            {
                isPhysicsDebugOpen = !isPhysicsDebugOpen;
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

    isHierarchyFocused = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows); // focus 확인

    if (ImGui::Button("Create GameObject"))
        SceneSystem::Instance().GetCurrentScene()->AddGameObjectByName("NewGameObject");

    auto scene = SceneSystem::Instance().GetCurrentScene();
    if (!scene) { ImGui::End(); return; }

    // 각 오브젝트 표시
    scene->ForEachGameObject([this](GameObject* obj)
    {
        Transform* tr = obj->GetComponent<Transform>();
        if (!tr) return;

        if (tr->GetParent() != nullptr) return; // 루트만

        DrawHierarchyNode(obj);
    });

    // 빈 공간을 dropspace로 만들기
    DrawHierarchyDropSpace();
    ImGui::End();
}

void Editor::DrawHierarchyNode(GameObject* obj)
{
    Transform* tr = obj->GetComponent<Transform>();
    if (!tr) return;

    ImGui::PushID(obj);

    const auto& children = tr->GetChildren();

    ImGuiTreeNodeFlags flags =
        ImGuiTreeNodeFlags_OpenOnArrow |
        ImGuiTreeNodeFlags_SpanAvailWidth;

    bool isLeaf = children.empty();
    if (isLeaf)
        flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;

    if (selectedObject == obj)
        flags |= ImGuiTreeNodeFlags_Selected;

    bool open = ImGui::TreeNodeEx(obj->GetName().c_str(), flags);

    // 클릭 선택
    if (ImGui::IsItemClicked())
        SelectObject(obj);

    // (1) Drag source
    if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID))
    {
        GameObject* payloadObj = obj;
        ImGui::SetDragDropPayload(kPayload_GameObject, &payloadObj, sizeof(GameObject*));
        ImGui::TextUnformatted(obj->GetName().c_str());
        ImGui::EndDragDropSource();
    }

    // (2) Drop target
    if (ImGui::BeginDragDropTarget())
    {
        // 오브젝트 이동
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(kPayload_GameObject))
        {
            GameObject* dragged = *(GameObject**)payload->Data;
            if (dragged && dragged != obj)
            {
                Transform* dtr = dragged->GetComponent<Transform>();
                if (dtr && dtr != tr)
                {
                    // 순환 참조 체크: obj가 dragged의 자손인지 확인
                    bool wouldCreateCycle = false;
                    Transform* ancestor = tr;
                    while (ancestor)
                    {
                        if (ancestor == dtr)
                        {
                            wouldCreateCycle = true;
                            break;
                        }
                        ancestor = ancestor->GetParent();
                    }

                    if (!wouldCreateCycle)
                    {
                        dtr->SetParent(tr);
                    }
                }
            }         
        }

        // 프리팹 -> 오브젝트 부모 연결 후 구성
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(kPayload_Prefab))
        {
            int prefabIndex = *(int*)payload->Data;
            if (prefabIndex >= 0 && prefabIndex < (int)prefabs.size())
            {
                Transform* parent = obj->GetComponent<Transform>();
                InstantiatePrefabFromJson(prefabs[prefabIndex].jsons, parent);
            }
        }

        ImGui::EndDragDropTarget();
    }

    // 자식 렌더링 (Leaf가 아니고 열려있을 때만)
    if (!isLeaf && open)
    {
        for (Transform* childTr : children)
        {
            if (!childTr) continue;
            GameObject* childObj = childTr->GetOwner();
            if (childObj)
                DrawHierarchyNode(childObj);
        }
        ImGui::TreePop();  // TreePush가 되었을 때만 Pop
    }

    ImGui::PopID();
}
void Editor::DrawHierarchyDropSpace()
{
    // 2. 빈 공간을 드롭 타겟으로 지정한다.
    ImVec2 avail = ImGui::GetContentRegionAvail(); // 창에서 사용 가능한 남은 공간

    if (avail.y < 1.0f) avail.y = 1.0f; // 최소 남은 공간 == 1.0f

    // 배경 전체(남은 영역)를 아이템으로 만든다
    ImGui::InvisibleButton("##HierarchyBlankSpace", avail,
        ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonRight);

    // DragDrop 확인
    if (ImGui::BeginDragDropTarget())
    {
        // 루트 빼기
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(kPayload_GameObject))
        {
            GameObject* dragged = *(GameObject**)payload->Data;
            if (dragged)
            {
                Transform* dtr = dragged->GetComponent<Transform>();
                if (dtr)
                {
                    dtr->RemoveSelfAtParent(); // 부모 해제
                }
            }
        }

        // 루트에 프리팹 생성
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(kPayload_Prefab))
        {
            int prefabIndex = *(int*)payload->Data;
            if (prefabIndex >= 0 && prefabIndex < (int)prefabs.size())
            {
                InstantiatePrefabFromJson(prefabs[prefabIndex].jsons, nullptr);
            }
        }

        ImGui::EndDragDropTarget();
    }

}

void Editor::RenderPrefabWindow(HWND& hwnd)
{
    if (!isPrefabWindowOpen) return;

    ImGui::Begin("Prefabs", &isPrefabWindowOpen);

    ImGui::TextUnformatted("Drag a GameObject here to register as a prefab.");
    ImGui::Separator();

    // (A) 등록 Drop Zone
    {
        ImVec2 avail = ImGui::GetContentRegionAvail();
        float zoneH = 80.0f;
        ImVec2 zoneSize(avail.x, zoneH);

        ImGui::InvisibleButton("##PrefabDropZone", zoneSize,
            ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonRight);

        // 시각적 안내 (간단)
        ImVec2 p0 = ImGui::GetItemRectMin();
        ImVec2 p1 = ImGui::GetItemRectMax();
        ImGui::GetWindowDrawList()->AddRect(p0, p1, IM_COL32(200, 200, 200, 255));
        ImGui::GetWindowDrawList()->AddText(ImVec2(p0.x + 8, p0.y + 8), IM_COL32(200, 200, 200, 255),
            "DROP HERE");

        if (ImGui::BeginDragDropTarget())
        {
            // 하이어라키에서 쓰는 payload 재사용: kPayload_GameObject
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(kPayload_GameObject))
            {
                GameObject* dragged = *(GameObject**)payload->Data;
                if (dragged && !dragged->IsDestory())
                {
                    std::vector<std::string> datas;
                    CollectSubtree(dragged, datas);

                    std::string base = dragged->GetName() + std::string("_Prefab");
                    std::string uniqueName = MakeUniquePrefabName(base, prefabs);

                    prefabs.push_back({ uniqueName, std::move(datas) });
                    selectedPrefabIndex = (int)prefabs.size() - 1;
                }
            }

            ImGui::EndDragDropTarget();
        }
    }

    ImGui::Separator();

    // (B) 프리팹 리스트
    ImGui::Text("Registered Prefabs: %d", (int)prefabs.size());
    if (ImGui::Button("Reload Prefabs From Folder")) // 파일 리로드 버튼
    {
        auto prefabsDir = PathHelper::FindDirectory("Assets\\Prefabs");
        if (prefabsDir.has_value())
        {
            std::string path = prefabsDir.value().string();
            LoadPrefabsFromFolder(path);
        }
        else
        {
            MessageBoxA(hwnd, "No directory found : ..\\Assets\\Prefabs", "Error", MB_OK | MB_ICONINFORMATION);
        }
    }

    ImGui::Spacing();

    // 선택/삭제 UI
    if (selectedPrefabIndex >= 0 && selectedPrefabIndex < (int)prefabs.size())
    {
        ImGui::SameLine();
        if (ImGui::Button("Save Prefab"))
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

            // GameWorld를 파일에 저장
            if (SavePrefabToJson(hwnd, prefabs[selectedPrefabIndex], szFile))
            {
                MessageBoxA(hwnd, "Scene saved successfully!", "Save", MB_OK | MB_ICONINFORMATION);
            }
            else
            {
                MessageBoxA(hwnd, "Failed to save scene!", "Error", MB_OK | MB_ICONERROR);
            }
        }
        else if (ImGui::Button("Delete Selected"))
        {
            prefabs.erase(prefabs.begin() + selectedPrefabIndex);
            if (prefabs.empty()) selectedPrefabIndex = -1;
            else selectedPrefabIndex = std::min(selectedPrefabIndex, (int)prefabs.size() - 1);
        }
    }

    ImGui::BeginChild("##PrefabList", ImVec2(0, 0), true);

    for (int i = 0; i < (int)prefabs.size(); ++i)
    {
        ImGui::PushID(i);

        bool selected = (i == selectedPrefabIndex);
        if (ImGui::Selectable(prefabs[i].name.c_str(), selected))
            selectedPrefabIndex = i;

        // (C) Drag source: Prefab -> Hierarchy
        if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID))
        {
            int payloadIndex = i;
            ImGui::SetDragDropPayload(kPayload_Prefab, &payloadIndex, sizeof(int));
            ImGui::Text("Prefab: %s", prefabs[i].name.c_str());
            ImGui::EndDragDropSource();
        }

        // 우클릭 메뉴(옵션): 이름 변경, 복제 등
        if (ImGui::BeginPopupContextItem("##PrefabContext"))
        {
            if (ImGui::MenuItem("Duplicate"))
            {
                std::string newName = MakeUniquePrefabName(prefabs[i].name, prefabs);
                prefabs.push_back({ newName, prefabs[i].jsons });
            }
            ImGui::EndPopup();
        }

        ImGui::PopID();
    }

    ImGui::EndChild();

    ImGui::End();
}

std::string Editor::MakeUniquePrefabName(const std::string& base, const std::vector<PrefabEntry>& list)
{
    // base가 이미 있으면 base(1), base(2)...
    auto exists = [&](const std::string& n) {
        for (auto& e : list) if (e.name == n) return true;
        return false;
        };

    if (!exists(base)) return base;

    for (int i = 1; i < 9999; ++i)
    {
        std::string cand = base + "(" + std::to_string(i) + ")";
        if (!exists(cand)) return cand;
    }
    return base + "(?)";
}

GameObject* Editor::InstantiatePrefabFromJson(const std::vector<std::string>& jsonStrs, Transform* attachParent)
{
    auto scene = SceneSystem::Instance().GetCurrentScene();
    if (!scene) return nullptr;

    if (jsonStrs.empty()) return nullptr;

    std::vector<GameObject*> created;
    created.reserve(jsonStrs.size());

    std::vector<int> parentIDs;      // 생성 순서대로 ParentID 저장 (-1 가능)
    parentIDs.reserve(jsonStrs.size());

    // 1) 전부 생성 + Deserialize + ID/ParentID 수집
    for (const std::string& s : jsonStrs)
    {
        nlohmann::json props = nlohmann::json::parse(s, nullptr, false);
        if (props.is_discarded()) continue;

        std::string objectName = "PrefabInstance";
        if (props.contains("Name")) objectName = props["Name"].get<std::string>();

        GameObject* instance = scene->AddGameObjectByName(objectName);
        if (!instance) continue;

        instance->Deserialize(props);

        if (props.contains("ID"))
            instance->SetId(props["ID"]);

        int p = -1;
        if (props.contains("ParentID"))
            p = props["ParentID"];
        parentIDs.push_back(p);

        created.push_back(instance);
    }

    if (created.empty()) return nullptr;

    // 2) 계층 재구성 (씬 로드 로직과 동일)
    // created[i] <-> parentIDs[i] 1:1 매칭
    if (!parentIDs.empty() && parentIDs.size() == created.size())
    {
        for (int i = 0; i < (int)created.size(); ++i)
        {
            int currParentID = parentIDs[i];
            if (currParentID == -1) continue;

            GameObject* currObject = created[i];
            if (!currObject) continue;

            for (int j = 0; j < (int)created.size(); ++j)
            {
                if (i == j) continue;
                if (!created[j]) continue;

                if (currParentID == created[j]->GetId())
                {
                    currObject->GetTransform()->SetParent(created[j]->GetTransform());
                    break;
                }
            }
        }
    }

    // 3) Hierarchy에 드랍한 경우: 루트들(ParentID == -1)만 attachParent 아래로
    // 생성한 오브젝트 끼리의ID여서 충돌날 일 없음
    if (attachParent)
    {
        for (int i = 0; i < (int)created.size(); ++i)
        {
            if (!created[i]) continue;
            if (parentIDs[i] == -1)
            {
                created[i]->GetTransform()->SetParent(attachParent);
            }
        }
    }

    // 3-1) 구성 후 ObjID 충돌 방지를 위한 ID 재구성
    for (int i = 0; i < (int)created.size(); ++i)
    {
        if (!created[i]) continue;

        created[i]->SetId(ObjectSystem::Instance().GetNewID()); // 새 ID 갱신
    }

    // 4) 반환값: 첫 번째 루트(ParentID == -1)를 대표로 반환
    for (int i = 0; i < (int)created.size(); ++i)
    {
        if (created[i] && parentIDs[i] == -1)
            return created[i];
    }
}

void Editor::CollectSubtree(GameObject* root, std::vector<std::string>& out)
{
    if (!root || root->IsDestory()) return;

    // Serialize가 root 전체를 준다면, properties만 쓰는 게 씬 로드 로직과 완전히 호환됨
    // - root->Serialize()가 {"type","properties"} 형태라고 가정
    nlohmann::json objData = root->Serialize();
    if (objData.contains("properties"))
    {
        out.push_back(objData["properties"].dump(2));
    }

    Transform* tr = root->GetTransform();
    if (!tr) return;

    for (Transform* c : tr->GetChildren())
    {
        if (!c) continue;
        GameObject* child = c->GetOwner();
        CollectSubtree(child, out);
    }
}

bool Editor::SavePrefabToJson(HWND& hwnd, PrefabEntry& data, const char* filePath)
{
    // prefabs
    //  obj1
    //  obj2
    //  ...

    nlohmann::json root;
    root["prefab"] = nlohmann::json::array();
    // 저장할 json 구성하기
    for (const std::string& s : data.jsons)
    {
        nlohmann::json props = nlohmann::json::parse(s, nullptr, false);
        if (props.is_discarded()) continue;
        root["prefab"].push_back(props); // 각 배열마다 오브젝트 내용 저장
    }

    // 파일 이름 -> 프리팹 이름
    std::ofstream file(filePath);
    if (!file.is_open()) return false;

    file << root.dump(2);
    file.close();

    return true;
}

void Editor::LoadPrefabsFromFolder(const std::string& folder)
{
    namespace fs = std::filesystem;

    prefabs.clear();
    selectedPrefabIndex = -1;

    fs::path dir(folder);
    if (!fs::exists(dir) || !fs::is_directory(dir))
        return;

    for (const auto& entry : fs::directory_iterator(dir))
    {
        if (!entry.is_regular_file()) continue;

        fs::path p = entry.path();
        if (p.extension() != ".json") continue; // .json 확장자인지 확ㅇ니

        PrefabEntry prefab;
        if (LoadPrefabFromJsonFile(p.string(), prefab))
        {
            prefab.name = MakeUniquePrefabName(prefab.name, prefabs);
            prefabs.push_back(prefab);
        }
    }

    if (!prefabs.empty())
        selectedPrefabIndex = 0;
}

bool Editor::LoadPrefabFromJsonFile(const std::string& filepath, PrefabEntry& outPrefab)
{
    std::ifstream file(filepath);
    if (!file.is_open()) return false;

    nlohmann::json root;
    try
    {
        file >> root;
    }
    catch (const nlohmann::json::exception&)
    {
        file.close();
        return false;
    }
    file.close();

    // 저장 포맷: { "prefab": [ {properties...}, {properties...}, ... ] }
    if (!root.contains("prefab") || !root["prefab"].is_array())
        return false;

    outPrefab.jsons.clear();
    for (const auto& props : root["prefab"])
    {
        if (!props.is_object()) continue;
        outPrefab.jsons.push_back(props.dump(2));
    }

    if (outPrefab.jsons.empty())
        return false;

    std::filesystem::path p(filepath);
    outPrefab.name = p.stem().string(); // stem : 일반 파일에서 확장자를 제거한 이름을 반환

    return true;
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
        SceneSystem::Instance().GetCurrentScene()->ReloadScene();
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

        Matrix camWorld = cam->GetOwner()->GetTransform()->GetWorldMatrix();
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
    if (!comp) return;

    rttr::type t = rttr::type::get(*comp);

    // 표시용 라벨 + ID 분리
    std::string headerLabel = t.get_name().to_string();
    std::string headerId = "##" + std::to_string((uintptr_t)comp);
    std::string header = headerLabel + headerId;

    // 헤더
    bool open = ImGui::CollapsingHeader(header.c_str(), ImGuiTreeNodeFlags_DefaultOpen);

    // 헤더 오른쪽에 Remove 버튼(Transform 제외)
    if (compName != "Transform")
    {
        // 같은 줄에 오른쪽으로 밀기 (대충)
        float avail = ImGui::GetContentRegionAvail().x;
        ImGui::SameLine(ImGui::GetCursorPosX() + avail - 110.0f);

        ImGui::PushID(comp);
        if (ImGui::SmallButton("Remove"))
        {
            selectedObject->RemoveComponent(comp);
            ImGui::PopID();
            return; // 삭제했으면 더 그리지 말기(댕글링 방지)
        }
        ImGui::PopID();
    }

    if (!open) return;

    // 내용은 헤더가 열렸을 때만
    if (compName == "Transform")
    {
        for (auto& prop : t.get_properties())
        {
            rttr::variant value = prop.get_value(*comp);
            std::string name = prop.get_name().to_string();
            if (!value.is_valid()) continue;

            if (value.is_type<DirectX::SimpleMath::Vector3>() && name == "Rotation")
            {
                auto rot = value.get_value<DirectX::SimpleMath::Vector3>();
                DirectX::SimpleMath::Vector3 eulerDegree =
                {
                    XMConvertToDegrees(rot.x),
                    XMConvertToDegrees(rot.y),
                    XMConvertToDegrees(rot.z)
                };

                if (ImGui::DragFloat3("Rotation", &eulerDegree.x, 0.1f))
                {
                    rot = {
                        XMConvertToRadians(eulerDegree.x),
                        XMConvertToRadians(eulerDegree.y),
                        XMConvertToRadians(eulerDegree.z)
                    };
                    prop.set_value(*comp, rot);

                    GameObject* owner = comp->GetOwner();
                    if (auto phys = owner->GetComponent<PhysicsComponent>())
                        phys->SyncToPhysics();
                }
            }
            else if (value.is_type<DirectX::SimpleMath::Vector3>())
            {
                auto vec = value.get_value<DirectX::SimpleMath::Vector3>();
                if (ImGui::DragFloat3(name.c_str(), &vec.x, 0.1f))
                {
                    prop.set_value(*comp, vec);

                    GameObject* owner = comp->GetOwner();
                    if (auto phys = owner->GetComponent<PhysicsComponent>())
                        phys->SyncToPhysics();
                    if (auto cct = owner->GetComponent<CharacterControllerComponent>())
                        cct->Teleport(vec);
                }
            }
        }
        return;
    }

    if (compName == "FBXData")
    {
        // FileDialog 키 유니크하게
        std::string keyNonStatic = "ChooseFileDlgKey##" + std::to_string((uintptr_t)comp);
        std::string keyStatic = "ChooseStaticFileDlgKey##" + std::to_string((uintptr_t)comp);

        for (auto& prop : t.get_properties())
        {
            rttr::variant value = prop.get_value(*comp);
            std::string name = prop.get_name().to_string();
            if (!value.is_valid()) continue;

            if (value.is_type<std::string>() && name == "DataPath")
            {
                std::string path = value.get_value<std::string>();
                ImGui::Text("Current Path: %s", path.c_str());

                if (ImGui::Button("Browse nonStatic"))
                {
                    IGFD::FileDialogConfig config;
                    config.path = "../";
                    ImGuiFileDialog::Instance()->OpenDialog(keyNonStatic, "Choose File", ".fbx,.glb", config);
                }

                if (ImGuiFileDialog::Instance()->Display(keyNonStatic))
                {
                    if (ImGuiFileDialog::Instance()->IsOk())
                    {
                        std::string filePathName = ImGuiFileDialog::Instance()->GetFilePathName();
                        std::filesystem::path relativePath = std::filesystem::relative(filePathName);
                        auto* fbx = dynamic_cast<FBXData*>(comp);
                        if (fbx) fbx->ChangeData(relativePath.string());
                    }
                    ImGuiFileDialog::Instance()->Close();
                }

                if (ImGui::Button("Browse static"))
                {
                    IGFD::FileDialogConfig config;
                    config.path = "../";
                    ImGuiFileDialog::Instance()->OpenDialog(keyStatic, "Choose File", ".fbx,.glb", config);
                }

                if (ImGuiFileDialog::Instance()->Display(keyStatic))
                {
                    if (ImGuiFileDialog::Instance()->IsOk())
                    {
                        std::string filePathName = ImGuiFileDialog::Instance()->GetFilePathName();
                        std::filesystem::path relativePath = std::filesystem::relative(filePathName);
                        auto* fbx = dynamic_cast<FBXData*>(comp);
                        if (fbx) fbx->ChangeStaticData(relativePath.string());
                    }
                    ImGuiFileDialog::Instance()->Close();
                }
            }
        }
        return;
    }

    if (compName == "Decal")
    {
        std::string keyTex = "ChooseDecalTexDlgKey##" + std::to_string((uintptr_t)comp);

        for (auto& prop : t.get_properties())
        {
            rttr::variant value = prop.get_value(*comp);
            std::string name = prop.get_name().to_string();
            if (!value.is_valid()) continue;

            if (value.is_type<std::string>() && name == "TexturePath")
            {
                std::string path = value.get_value<std::string>();
                ImGui::Text("Current Path: %s", path.c_str());

                if (ImGui::Button("Browse"))
                {
                    IGFD::FileDialogConfig config;
                    config.path = "../";
                    ImGuiFileDialog::Instance()->OpenDialog(keyTex, "Choose File", ".png,.tga", config);
                }

                if (ImGuiFileDialog::Instance()->Display(keyTex))
                {
                    if (ImGuiFileDialog::Instance()->IsOk())
                    {
                        std::string filePathName = ImGuiFileDialog::Instance()->GetFilePathName();
                        std::filesystem::path relativePath = std::filesystem::relative(filePathName);
                        auto* decal = dynamic_cast<Decal*>(comp);
                        if (decal) decal->ChangeData(relativePath.string());
                    }
                    ImGuiFileDialog::Instance()->Close();
                }
            }
        }

        ReadVariants(*comp);
        return;
    }

    // 기본
    ImGui::PushID(comp);
    ReadVariants(*comp);
    ImGui::PopID();
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


    //// 선택된 오브젝트는 밝은 초록색
    //SceneSystem::Instance().GetCurrentScene()->ForEachGameObject([&](GameObject* gameObject) {
    //    if (gameObject->IsDestory()) return;

    //    XMVECTOR color = XMVectorSet(0.0f, 1.0f, 0.0f, 1.0f);
    //    DebugDraw::g_Batch->Begin();
    //    DebugDraw::Draw(DebugDraw::g_Batch.get(), gameObject->GetAABB(), color);
    //    DebugDraw::g_Batch->End();
    // });

     // ===============================
    // Debug Draw Begin
    // ===============================
    DebugDraw::g_Batch->Begin();

    // AABB
    SceneSystem::Instance().GetCurrentScene()->ForEachGameObject([&](GameObject* gameObject)
        {
            if (gameObject->IsDestory()) return;
            if (gameObject->GetComponent<FBXRenderer>() != nullptr) return;

            XMVECTOR color = XMVectorSet(0.0f, 1.0f, 0.0f, 1.0f);
            BoundingBox box = gameObject->GetAABB();
            DebugDraw::Draw(DebugDraw::g_Batch.get(), box, color);
        });

    // PhysX
    if (isPhysicsDebugOpen)
    {
        PhysicsSystem::Instance().DrawPhysXActors();
    }

    // ===============================
    // Debug Draw End
    // ===============================
    DebugDraw::g_Batch->End();
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
    	MessageBoxA(hwnd, "Failed to load scene! object or world data not found.", "Error", MB_OK | MB_ICONERROR);
    }
}

void Editor::CreatePickingStagingTex()
{
    D3D11_TEXTURE2D_DESC desc = {};
    desc.Width = screenWidth;
    desc.Height = screenHeight;
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.Format = DXGI_FORMAT_R32_UINT;
    desc.SampleDesc.Count = 1; 
    desc.SampleDesc.Quality = 0;
    desc.Usage = D3D11_USAGE_STAGING;
    desc.BindFlags = 0;
    desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    desc.MiscFlags = 0;

    HR_T(device->CreateTexture2D(&desc, nullptr, coppedPickingTex.ReleaseAndGetAddressOf()));
}

void Editor::CheckObjectPicking()
{
    isMouseLeftClick = false;
    auto mouse = DirectX::Mouse::Get().GetState();
    static bool lastMouseLeft = false;

    bool currMouseLeft = mouse.leftButton;
    isMouseLeftClick = (!lastMouseLeft && currMouseLeft);
    lastMouseLeft = currMouseLeft;

    mouseXY = { mouse.x, mouse.y };

    ImGuiIO& io = ImGui::GetIO();

    bool allowWorldPick =
        !io.WantCaptureMouse       // ImGui가 마우스를 쓰는 중이면 차단
        && !io.WantTextInput;      // (선택) 텍스트 입력 중이면 차단

    if (isMouseLeftClick && allowWorldPick && !isAABBPicking)
    {
        auto& sm = ShaderManager::Instance();
        context->CopyResource(coppedPickingTex.Get(), sm.pickingTex.Get()); // 기록된 값 가져오기

        D3D11_MAPPED_SUBRESOURCE mapped;
        context->Map(coppedPickingTex.Get(), 0, D3D11_MAP_READ, 0, &mapped); // Map : 하위 리소스에 대한 포인터 가져오기

        uint32_t* row = (uint32_t*)((uint8_t*)mapped.pData + mouseXY.y * mapped.RowPitch); // 마우스값의 row 
        currPickedID = row[mouseXY.x] - 1;	// x, y 좌표에 있는 ID 찾기

        context->Unmap(coppedPickingTex.Get(), 0);

        auto scene = SceneSystem::Instance().GetCurrentScene();
        SelectObject(scene->GetGameObjectByIndex(static_cast<int>(currPickedID + 1)));
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

        if (value.get_type().is_enumeration())
        {
            rttr::type enumType = value.get_type();
            rttr::enumeration e = enumType.get_enumeration();

            // 현재 선택된 항목 이름
            std::string currentName;
            {
                rttr::variant cur = value; // 현재 enum 값
                rttr::string_view sv = e.value_to_name(cur);
                currentName = sv.empty() ? std::string("<invalid>") : sv.to_string();
            }

            // 모든 enum 이름 리스트
            auto names = e.get_names(); // array_range<string_view>
            if (!names.empty())
            {
                // currentName이 names 중 몇 번째인지 찾기
                int currentIndex = 0;
                int idx = 0;
                for (auto n : names)
                {
                    if (n.to_string() == currentName) // 선택한 인덱스 찾기
                    {
                        currentIndex = idx;
                        break;
                    }
                    ++idx;
                }

                // ImGui Combo
                const char* preview = currentName.c_str();
                if (ImGui::BeginCombo(name.c_str(), preview))
                {
                    int i = 0;
                    for (auto n : names)
                    {
                        std::string itemName = n.to_string();
                        bool selected = (i == currentIndex);
                        if (ImGui::Selectable(itemName.c_str(), selected))
                        {
                            // 이름 -> enum 값 variant
                            rttr::variant newVal = e.name_to_value(n);
                            if (newVal.is_valid())
                            {
                                // prop이 enum 타입이면 그대로 set_value
                                prop.set_value(inst, newVal);
                            }
                        }
                        if (selected)
                            ImGui::SetItemDefaultFocus();
                        ++i;
                    }
                    ImGui::EndCombo();
                }
            }
        }
        else if (metaBool.is_valid() && metaBool.to_bool())
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
        else if (value.is_type<Vector2>())
        {
            Vector2 vec = value.get_value<SimpleMath::Vector2>();
            if (ImGui::DragFloat2(name.c_str(), &vec.x, 0.1f))
                prop.set_value(inst, vec);
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

void Editor::CheckObjectDeleteKey()
{
    // Hieararchy가 선택되었을 때만 제거 단축키 사용 가능

    if (!isHierarchyFocused) return;
    if (!selectedObject) return;
    if (selectedObject->IsDestory()) { selectedObject = nullptr; return; }

    ImGuiIO& io = ImGui::GetIO();

    // UI가 키보드 입력을 쓰고 있으면 삭제 금지 (텍스트 입력/단축키 충돌 방지)
    if (io.WantTextInput)
        return;

    if (!ImGui::IsWindowFocused(ImGuiFocusedFlags_AnyWindow))
        return;

    // 플레이 모드일 때 막기
    if (PlayModeSystem::Instance().IsPlaying())
        return;

    // imgui로 키 입력 확인
    if (ImGui::IsKeyPressed(ImGuiKey_Delete, false))
    {
        GameObject* victim = selectedObject;

        if (victim->GetComponent<Camera>() && CameraSystem::Instance().GetAllCamera().size() == 1)
        {
            MessageBoxA(NULL, "Scene need at least one camera.", "Delete not allowed", 0);
            return;
        }

        selectedObject = nullptr;
        victim->Destory();
    }
}

void Editor::OnInputProcess(const Keyboard::State &KeyState, const Keyboard::KeyboardStateTracker &KeyTracker, const Mouse::State &MouseState, const Mouse::ButtonStateTracker &MouseTracker)
{
    isAABBPicking = false;

    if (MouseTracker.leftButton == Mouse::ButtonStateTracker::PRESSED)
    {
        if (!ImGui::GetIO().WantCaptureMouse)
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

            if (hitObject != nullptr && hitObject->GetComponent<FBXRenderer>() == nullptr)
            {
                SelectObject(hitObject);
                isAABBPicking = true;
            }
        }
    }
}
#endif