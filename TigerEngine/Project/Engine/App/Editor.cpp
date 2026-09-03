#if _DEBUG
#include "Editor.h"
#include "../EngineSystem/CameraSystem.h"
#include "../Manager/ComponentFactory.h"
#include "../EngineSystem/SceneSystem.h"

#include <commdlg.h>
#include "imguiFileDialog/ImGuiFileDialog.h"
#include "../Components/FBXData.h"
#include "../Components/Decal.h"
#include "../RenderPass/ParticleSource/Effect.h"
#include "../Object/GameObject.h"
#include "../Util/DebugDraw.h"
#include "../Manager/WorldManager.h"
#include "../Manager/AudioManager.h"
#include "../Manager/Shadermanager.h"
#include "../EngineSystem/PlayModeSystem.h"
#include "../Components/Camera.h"
#include "../EngineSystem/PhysicsSystem.h"
#include "../EngineSystem/GridSystem.h"
#include "../Components/CharacterControllerComponent.h"
#include "../Components/GridComponent.h"

#include "Datas/ReflectionMedtaDatas.hpp"

#include "../Components/FBXRenderer.h"
#include "../Util/PathHelper.h"
#include "../Components/UI/Image.h"
#include "../Components/VisionComponent.h"
#include "../Components/UI/TextUI.h"
#include "../Util/EncodeConvertHelper.h"
#include "../Manager/UIManager.h"
#include "../Base/System/TimeSystem.h"

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

    currScene->ForEachGameObject([](GameObject* obj) {
        if (obj->GetName() == "FreeCamera") return;
        obj->UpdateAABB();
        });

    CheckObjectPicking();
    CheckObjectDeleteKey();
}

void Editor::Render(HWND& hwnd)
{
    ImGuizmo::BeginFrame();

    RenderMenuBar(hwnd);
    RenderHierarchy();
    RenderInspector();
    RenderDebugAABBDraw();
    // RenderCameraFrustum();
    RenderWorldSettings();
    RenderShadowMap();
    RenderPrefabWindow(hwnd);
    RenderCameraPanel();
    RenderGizmoSettings();
    // RenderWorldGrid();
    RenderGizmo();
    RenderVariantDebugger();

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
            else if (ImGui::MenuItem("Load scene"))
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
            if (ImGui::MenuItem("Game Object AABB"))
            {
                isGameObjectAABBDebugOpen = !isGameObjectAABBDebugOpen;
            }
            if (ImGui::MenuItem("Physics Collider"))
            {
                isPhysicsDebugOpen = !isPhysicsDebugOpen;
            }
            if (ImGui::MenuItem("Grid Debug"))
            {
                isGridDebugOpen = !isGridDebugOpen;
            }
            if (ImGui::MenuItem("Game UI", nullptr, isGameUIVisible))
            {
                isGameUIVisible = !isGameUIVisible;
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
        if (ImGui::BeginMenu("Camera"))
        {
            if (ImGui::MenuItem("Camera Setting"))
            {
                isCameraPanelOepn = !isCameraPanelOepn;
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Debugger"))
        {
            if (ImGui::MenuItem("Window n GameInfo"))
            {
                isOpenVariantDebugger = !isOpenVariantDebugger;
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

    if (avail.y < 200.0f) avail.y = 200.0f; // 최소 남은 공간 == 1.0f

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

    return nullptr;
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
        if (selectedObject == nullptr)
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
                    else if (value.is_type<bool>())
                    {
                        bool active = value.get_value<bool>();
                        if (ImGui::Checkbox("Active", &active))
                        {
                            prop.set_value(obj, active);
                        }
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
                    DrawAddComponentPopup(obj);
                    ImGui::EndPopup();
                }

                /* ------------------------------- 컴포넌트 내용 출력 ------------------------------- */
                for (auto& comp : obj->GetComponents())
                {
                    auto& registered = ComponentFactory::Instance().GetRegisteredComponents();
                    auto name = comp->GetName();

                    if (auto it = registered.find(name); it != registered.end())
                    {
                        ImGui::PushID(comp);
                        RenderComponentInfo(name, comp);
                        ImGui::NewLine();
                        ImGui::Separator();
                        ImGui::PopID();
                    }
                } // 컴포넌트 내용 출력 end
            } // obj is destroy end
        }
    }
    ImGui::End();
}

static const char* CatName(ComponentCategory c)
{
    switch (c)
    {
    case ComponentCategory::Core:      return "Core";
    case ComponentCategory::Rendering: return "Rendering";
    case ComponentCategory::Audio:     return "Audio";
    case ComponentCategory::Physics:   return "Physics";
    case ComponentCategory::Animation: return "Animation";
    case ComponentCategory::Script:    return "Scripts";
    case ComponentCategory::UI:        return "UI";
    default:                           return "Others";
    }
}

void Editor::DrawAddComponentPopup(GameObject* obj)
{
    auto& entries = ComponentFactory::Instance().GetRegisteredComponents();
    // entries: unordered_map<string, ComponentEntry> 라고 가정

    // bucket 만들기 (한 번만)
    std::map<ComponentCategory, std::vector<const ComponentEntry*>> buckets;
    for (auto& [k, e] : entries)
        buckets[e.category].push_back(&e);

    for (auto& [cat, list] : buckets)
    {
        // Transform 같은 금지 항목만 있는 카테고리는 숨기고 싶으면 여기서 필터링 가능

        if (ImGui::BeginMenu(CatName(cat)))
        {
            std::sort(list.begin(), list.end(),
                [](auto a, auto b) { return a->name < b->name; });

            for (auto* e : list)
            {
                if (e->name == "Transform") continue;

                if (ImGui::MenuItem(e->name.c_str()))
                {
                    e->creator(obj);

                    ImGui::CloseCurrentPopup();
                }
            }
            ImGui::EndMenu();
        }
    }

    ImGui::Separator();
    if (ImGui::MenuItem("Close"))
        ImGui::CloseCurrentPopup();
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
        CameraSystem::Instance().NextCamera();
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
            CameraSystem::Instance().SetCurrCameraToFreeCamera();
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
        CameraSystem::Instance().SetCurrCameraToFreeCamera();
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

void Editor::RenderGizmoSettings()
{
    if (!ImGui::Begin("Gizmo"))
    {
        ImGui::End();
        return;
    }

    ImGui::Checkbox("Enable Gizmo", &isGizmoEnabled);
    ImGui::Checkbox("World Grid", &isWorldGridEnabled);
    ImGui::DragFloat("Grid Size", &worldGridSize, 0.1f, 0.1f, 1000.0f);

    ImGui::Separator();

    if (ImGui::RadioButton("Translate (W)", gizmoOperation == ImGuizmo::TRANSLATE))
        gizmoOperation = ImGuizmo::TRANSLATE;
    ImGui::SameLine();
    if (ImGui::RadioButton("Rotate (E)", gizmoOperation == ImGuizmo::ROTATE))
        gizmoOperation = ImGuizmo::ROTATE;
    ImGui::SameLine();
    if (ImGui::RadioButton("Scale (R)", gizmoOperation == ImGuizmo::SCALE))
        gizmoOperation = ImGuizmo::SCALE;

    if (gizmoOperation != ImGuizmo::SCALE)
    {
        if (ImGui::RadioButton("Local", gizmoMode == ImGuizmo::LOCAL))
            gizmoMode = ImGuizmo::LOCAL;
        ImGui::SameLine();
        if (ImGui::RadioButton("World", gizmoMode == ImGuizmo::WORLD))
            gizmoMode = ImGuizmo::WORLD;
    }

    ImGui::Checkbox("Snap", &useGizmoSnap);
    if (useGizmoSnap)
    {
        if (gizmoOperation == ImGuizmo::TRANSLATE)
        {
            ImGui::DragFloat3("Snap (Move)", &snapTranslation.x, 0.1f, 0.0f, 1000.0f);
        }
        else if (gizmoOperation == ImGuizmo::ROTATE)
        {
            ImGui::DragFloat("Snap (Rotate)", &snapRotation, 0.1f, 0.0f, 360.0f);
        }
        else if (gizmoOperation == ImGuizmo::SCALE)
        {
            ImGui::DragFloat("Snap (Scale)", &snapScale, 0.01f, 0.0f, 100.0f);
        }
    }

    ImGuiIO& io = ImGui::GetIO();
    if (!io.WantCaptureKeyboard && !io.WantTextInput)
    {
        if (ImGui::IsKeyPressed(ImGuiKey_W))
            gizmoOperation = ImGuizmo::TRANSLATE;
        if (ImGui::IsKeyPressed(ImGuiKey_E))
            gizmoOperation = ImGuizmo::ROTATE;
        if (ImGui::IsKeyPressed(ImGuiKey_R))
            gizmoOperation = ImGuizmo::SCALE;
        if (ImGui::IsKeyPressed(ImGuiKey_Q))
            gizmoMode = (gizmoMode == ImGuizmo::LOCAL) ? ImGuizmo::WORLD : ImGuizmo::LOCAL;
    }

    ImGui::End();
}

void Editor::RenderWorldGrid()
{
    if (!isWorldGridEnabled)
        return;

    Camera* cam = CameraSystem::Instance().GetFreeCamera();
    if (!cam)
        return;

    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGuizmo::SetDrawlist(ImGui::GetForegroundDrawList());
    ImGuizmo::SetRect(viewport->Pos.x, viewport->Pos.y, viewport->Size.x, viewport->Size.y);

    Matrix view = cam->GetView();
    Matrix projection = cam->GetProjection();
    Matrix identity = Matrix::Identity;

    ImGuizmo::DrawGrid(reinterpret_cast<const float*>(&view), reinterpret_cast<const float*>(&projection),
        reinterpret_cast<const float*>(&identity), worldGridSize);
}

void Editor::RenderGizmo()
{
    if (!isGizmoEnabled)
        return;
    if (!selectedObject || selectedObject->IsDestory())
        return;
    if (PlayModeSystem::Instance().IsPlaying())
        return;

    Transform* transform = selectedObject->GetTransform();
    if (!transform)
        return;

    Camera* cam = CameraSystem::Instance().GetFreeCamera();
    if (!cam)
        return;

    Matrix view = cam->GetView();
    Matrix projection = cam->GetProjection();
    Matrix world = transform->GetWorldMatrix();

    float matrix[16];
    memcpy(matrix, &world, sizeof(matrix));

    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGuizmo::SetDrawlist(ImGui::GetForegroundDrawList());
    ImGuizmo::SetRect(viewport->Pos.x, viewport->Pos.y, viewport->Size.x, viewport->Size.y);
    ImGuizmo::SetOrthographic(false);

    float snapValues[3] = {};
    const float* snapPtr = nullptr;
    if (useGizmoSnap)
    {
        if (gizmoOperation == ImGuizmo::TRANSLATE)
        {
            snapValues[0] = snapTranslation.x;
            snapValues[1] = snapTranslation.y;
            snapValues[2] = snapTranslation.z;
            snapPtr = snapValues;
        }
        else if (gizmoOperation == ImGuizmo::ROTATE)
        {
            snapValues[0] = snapRotation;
            snapValues[1] = snapRotation;
            snapValues[2] = snapRotation;
            snapPtr = snapValues;
        }
        else if (gizmoOperation == ImGuizmo::SCALE)
        {
            snapValues[0] = snapScale;
            snapValues[1] = snapScale;
            snapValues[2] = snapScale;
            snapPtr = snapValues;
        }
    }

    bool manipulated = ImGuizmo::Manipulate(
        reinterpret_cast<const float*>(&view),
        reinterpret_cast<const float*>(&projection),
        gizmoOperation,
        gizmoMode,
        matrix,
        nullptr,
        snapPtr
    );

    if (manipulated && ImGuizmo::IsUsing())
    {
        Matrix newWorld;
        memcpy(&newWorld, matrix, sizeof(matrix));
        ApplyGizmoToTransform(transform, newWorld);
    }
}

void Editor::ApplyGizmoToTransform(Transform* transform, const Matrix& worldMatrix)
{
    if (!transform)
        return;

    Matrix localMatrix = worldMatrix;
    if (Transform* parent = transform->GetParent())
    {
        Matrix parentWorld = parent->GetWorldMatrix();
        Matrix invParent = parentWorld.Invert();
        localMatrix = worldMatrix * invParent;
    }

    DirectX::XMVECTOR scale;
    DirectX::XMVECTOR rot;
    DirectX::XMVECTOR pos;
    if (DirectX::XMMatrixDecompose(&scale, &rot, &pos, localMatrix))
    {
        transform->SetScale(Vector3(scale));
        transform->SetQuaternion(Quaternion(rot));
        transform->SetPosition(Vector3(pos));

        GameObject* owner = transform->GetOwner();
        if (auto phys = owner->GetComponent<PhysicsComponent>())
            phys->SyncToPhysics();
        if (auto cct = owner->GetComponent<CharacterControllerComponent>())
            cct->Teleport(Vector3(pos));
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
        // float avail = ImGui::GetContentRegionAvail().x;
        // ImGui::SameLine(ImGui::GetCursorPosX() + avail - 110.0f);

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

        ImGui::PushID(comp);
        ReadVariants(*comp);
        ImGui::PopID();
        return;
    }

    if (compName == "Image")
    {
        for (auto& prop : t.get_properties())
        {
            std::string keyUITex = "ChooseUITexDlgKey##" + std::to_string((uintptr_t)comp);
            rttr::variant value = prop.get_value(*comp);
            std::string name = prop.get_name().to_string();
            if (!value.is_valid()) continue;

            if (value.is_type<std::string>() && name == "path")
            {
                std::string path = value.get_value<std::string>();
                ImGui::Text("Current Image Path: %s", path.c_str());

                if (ImGui::Button("Browse texture"))
                {
                    IGFD::FileDialogConfig config;
                    config.path = "../";
                    ImGuiFileDialog::Instance()->OpenDialog(keyUITex, "Choose File", ".png, .jpg", config);
                }

                if (ImGuiFileDialog::Instance()->Display(keyUITex))
                {
                    if (ImGuiFileDialog::Instance()->IsOk())
                    {
                        std::string filePathName = ImGuiFileDialog::Instance()->GetFilePathName();
                        std::filesystem::path relativePath = std::filesystem::relative(filePathName);
                        auto* image = dynamic_cast<Image*>(comp);
                        if (image) image->ChangeData(relativePath.string());
                    }
                    ImGuiFileDialog::Instance()->Close();
                }
            }
        }
    }

    if (compName == "AudioSourceComponent" || compName == "AudioManagerComponent")
    {
        for (auto& prop : t.get_properties())
        {
            std::string name = prop.get_name().to_string();
            if (name.find("ClipId") != std::string::npos)
            {
                rttr::variant value = prop.get_value(*comp);
                if (value.is_valid() && value.is_type<std::string>())
                {
                    std::string s = value.get_value<std::string>();
                    auto ids = AudioManager::Instance().GetEntryIds();
                    ids.insert(ids.begin(), "<none>");

                    int currentIndex = 0;
                    for (int i = 1; i < static_cast<int>(ids.size()); ++i)
                    {
                        if (ids[i] == s)
                        {
                            currentIndex = i;
                            break;
                        }
                    }

                    const char* preview = ids[currentIndex].c_str();
                    std::string comboId = "AudioClipId##" + std::to_string((uintptr_t)comp) + "_" + name;
                    if (ImGui::BeginCombo(comboId.c_str(), preview))
                    {
                        for (int i = 0; i < static_cast<int>(ids.size()); ++i)
                        {
                            bool selected = (i == currentIndex);
                            if (ImGui::Selectable(ids[i].c_str(), selected))
                            {
                                if (i == 0)
                                {
                                    prop.set_value(*comp, std::string());
                                }
                                else
                                {
                                    prop.set_value(*comp, ids[i]);
                                }
                            }
                            if (selected)
                                ImGui::SetItemDefaultFocus();
                        }
                        ImGui::EndCombo();
                    }
                }
            }
        }
    }

    if (compName == "Effect")
    {
        auto* fx = dynamic_cast<Effect*>(comp);
        if (!fx)
        {
            ImGui::PushID(comp);
            ReadVariants(*comp);
            ImGui::PopID();
            return;
        }

        ImGui::PushID(comp);

        if (ImGui::Button("Play"))
            fx->Play();
        ImGui::SameLine();
        if (ImGui::Button("Stop"))
            fx->Stop();

        ImGui::Separator();
        ImGui::Text("Emitters");

        if (ImGui::Button("Add Emitter"))
            fx->emitters.emplace_back();

        for (size_t i = 0; i < fx->emitters.size(); )
        {
            Emitter& em = fx->emitters[i];
            ImGui::PushID(static_cast<int>(i));

            std::string header = "Emitter " + std::to_string(i);
            bool openEmitter = ImGui::CollapsingHeader(header.c_str(), ImGuiTreeNodeFlags_DefaultOpen);

            ImGui::SameLine();
            if (ImGui::SmallButton("Remove"))
            {
                fx->emitters.erase(fx->emitters.begin() + static_cast<std::ptrdiff_t>(i));
                ImGui::PopID();
                continue;
            }

            if (openEmitter)
            {
                // basic
                ImGui::Checkbox("enabled", &em.enabled);
                ImGui::SameLine();
                ImGui::Checkbox("playing", &em.playing);
                ImGui::DragFloat3("localOffset", &em.localOffset.x, 0.1f);

                // billboard
                {
                    const char* items[] = { "ScreenFacing", "YAxis" };
                    int v = (em.billboard == BillboardType::ScreenFacing) ? 0 : 1;
                    if (ImGui::Combo("billboard", &v, items, IM_ARRAYSIZE(items)))
                        em.billboard = (v == 0) ? BillboardType::ScreenFacing : BillboardType::YAxis;
                }

                // sprite sheet
                if (ImGui::TreeNode("SpriteSheet"))
                {
                    const bool hasTex = (em.sheet.resource && em.sheet.resource->srv);
                    if (hasTex)
                    {
                        ImGui::Text("Texture: Loaded");
                        ImGui::Image(
                            (ImTextureID)em.sheet.resource->srv.Get(),
                            ImVec2(64, 64));
                        ImGui::SameLine();
                        ImGui::Text("(%.0fx%.0f)", em.sheet.resource->texSizePx.x, em.sheet.resource->texSizePx.y);
                    }
                    else
                    {
                        ImGui::Text("Texture: <none>");
                    }

                    // texture path
                    {
                        std::string path = em.sheet.texturePath;
                        if (ImGui::InputText(
                            "texturePath",
                            path.data(),
                            path.capacity() + 1,
                            ImGuiInputTextFlags_CallbackResize,
                            [](ImGuiInputTextCallbackData* data) -> int
                            {
                                if (data->EventFlag == ImGuiInputTextFlags_CallbackResize)
                                {
                                    auto* str = static_cast<std::string*>(data->UserData);
                                    str->resize(data->BufTextLen);
                                    data->Buf = str->data();
                                }
                                return 0;
                            },
                            &path))
                        {
                            em.sheet.texturePath = path;
                        }

                        ImGui::SameLine();
                        if (ImGui::SmallButton("Load##SpriteSheet"))
                        {
                            if (!em.sheet.texturePath.empty())
                                em.sheet.SetPath(em.sheet.texturePath);
                        }
                    }

                    std::string keyFxTex = "ChooseFxTexDlgKey##" + std::to_string((uintptr_t)comp) + "_" + std::to_string(i);
                    if (ImGui::Button("Browse Texture"))
                    {
                        IGFD::FileDialogConfig config;
                        config.path = "../";
                        ImGuiFileDialog::Instance()->OpenDialog(keyFxTex, "Choose File", ".png,.jpg,.jpeg,.tga", config);
                    }

                    if (ImGuiFileDialog::Instance()->Display(keyFxTex))
                    {
                        if (ImGuiFileDialog::Instance()->IsOk())
                        {
                            std::string filePathName = ImGuiFileDialog::Instance()->GetFilePathName();
                            std::filesystem::path relativePath = std::filesystem::relative(filePathName);
                            em.sheet.SetPath(relativePath.string());
                        }
                        ImGuiFileDialog::Instance()->Close();
                    }

                    if (ImGui::Button("Clear Texture"))
                    {
                        em.sheet.texturePath.clear();
                        em.sheet.resource.reset();
                    }

                    em.sheet.cols = std::max(1, em.sheet.cols);
                    em.sheet.rows = std::max(1, em.sheet.rows);
                    ImGui::DragInt("cols", &em.sheet.cols, 1.0f, 1, 256);
                    ImGui::DragInt("rows", &em.sheet.rows, 1.0f, 1, 256);

                    int maxFrames = std::max(1, em.sheet.cols * em.sheet.rows);
                    em.sheet.frameCount = std::clamp(em.sheet.frameCount, 1, maxFrames);
                    ImGui::DragInt("frameCount", &em.sheet.frameCount, 1.0f, 1, maxFrames);

                    em.sheet.fps = std::max(0.0f, em.sheet.fps);
                    ImGui::DragFloat("fps", &em.sheet.fps, 0.1f, 0.0f, 240.0f);

                    em.sheet.baseSizeScale = std::max(0.001f, em.sheet.baseSizeScale);
                    ImGui::DragFloat("baseSizeScale", &em.sheet.baseSizeScale, 0.01f, 0.001f, 100.0f);
                    ImGui::Checkbox("sheet.loop", &em.sheet.loop);

                    if (em.sheet.fps > 0.0f)
                        ImGui::Text("flipbookDuration: %.3f", em.sheet.frameCount / em.sheet.fps);
                    else
                        ImGui::Text("flipbookDuration: <inf>");

                    ImGui::TreePop();
                }

                // emission
                ImGui::Separator();
                ImGui::Text("Emission");
                ImGui::DragFloat("duration", &em.duration, 0.05f, 0.0f, 99999.0f);
                ImGui::Checkbox("looping", &em.looping);
                ImGui::DragFloat("emitRate", &em.emitRate, 0.05f, 0.0f, 99999.0f);
                ImGui::DragInt("burstCount", &em.burstCount, 1.0f, 0, 99999);
                em.maxParticles = std::max(1, em.maxParticles);
                ImGui::DragInt("maxParticles", &em.maxParticles, 1.0f, 1, 99999);

                // particle mode
                {
                    const char* items[] = { "Dynamic", "Fixed" };
                    int v = (em.particleMode == ParticleMode::Dynamic) ? 0 : 1;
                    if (ImGui::Combo("particleMode", &v, items, IM_ARRAYSIZE(items)))
                        em.particleMode = (v == 0) ? ParticleMode::Dynamic : ParticleMode::Fixed;
                }

                if (em.particleMode == ParticleMode::Dynamic)
                {
                    if (ImGui::TreeNode("Dynamic"))
                    {
                        // spawn dynamic
                        ImGui::DragFloat("lifeMin", &em.dynamicData.lifeMin, 0.01f, 0.0f, 99999.0f);
                        ImGui::DragFloat("lifeMax", &em.dynamicData.lifeMax, 0.01f, 0.0f, 99999.0f);
                        em.dynamicData.lifeMax = std::max(em.dynamicData.lifeMin, em.dynamicData.lifeMax);

                        ImGui::DragFloat("rotationMin", &em.dynamicData.rotationMin, 0.01f, -99999.0f, 99999.0f);
                        ImGui::DragFloat("rotationMax", &em.dynamicData.rotationMax, 0.01f, -99999.0f, 99999.0f);
                        em.dynamicData.rotationMax = std::max(em.dynamicData.rotationMin, em.dynamicData.rotationMax);

                        ImGui::DragFloat2("sizeMin", &em.dynamicData.sizeMin.x, 0.01f, 0.0f, 99999.0f);
                        ImGui::DragFloat2("sizeMax", &em.dynamicData.sizeMax.x, 0.01f, 0.0f, 99999.0f);
                        em.dynamicData.sizeMax.x = std::max(em.dynamicData.sizeMin.x, em.dynamicData.sizeMax.x);
                        em.dynamicData.sizeMax.y = std::max(em.dynamicData.sizeMin.y, em.dynamicData.sizeMax.y);

                        ImGui::DragFloat("speedMin", &em.dynamicData.speedMin, 0.01f, 0.0f, 99999.0f);
                        ImGui::DragFloat("speedMax", &em.dynamicData.speedMax, 0.01f, 0.0f, 99999.0f);
                        em.dynamicData.speedMax = std::max(em.dynamicData.speedMin, em.dynamicData.speedMax);

                        ImGui::DragFloat("angularMin", &em.dynamicData.angularMin, 0.01f, -99999.0f, 99999.0f);
                        ImGui::DragFloat("angularMax", &em.dynamicData.angularMax, 0.01f, -99999.0f, 99999.0f);
                        em.dynamicData.angularMax = std::max(em.dynamicData.angularMin, em.dynamicData.angularMax);

                        ImGui::ColorEdit4("colorMin", &em.dynamicData.colorMin.x);
                        ImGui::ColorEdit4("colorMax", &em.dynamicData.colorMax.x);

                        // velocity shape
                        {
                            const char* items[] = { "Sphere", "Directional", "Cone", "Disk" };
                            int v = static_cast<int>(em.velocityShape);
                            if (ImGui::Combo("velocityShape", &v, items, IM_ARRAYSIZE(items)))
                                em.velocityShape = static_cast<VelocityShape>(v);
                        }

                        ImGui::DragFloat3("emitDir", &em.emitDir.x, 0.01f, -1.0f, 1.0f);
                        ImGui::DragFloat("coneAngleDeg", &em.coneAngleDeg, 0.1f, 0.0f, 180.0f);

                        ImGui::TreePop();
                    }
                }
                else // Fixed
                {
                    if (ImGui::TreeNode("Fixed"))
                    {
                        // spawn fixed
                        ImGui::DragFloat("rotation", &em.fixedData.rotation, 0.01f, -99999.0f, 99999.0f);
                        ImGui::DragFloat2("size", &em.fixedData.size.x, 0.01f, 0.0f, 99999.0f);
                        ImGui::ColorEdit4("startColor", &em.fixedData.startColor.x);

                        // flipbook play mode
                        {
                            const char* items[] = { "Once_Then_Die", "Once_Then_Hold", "Loop" };
                            int v = static_cast<int>(em.filpbookPlayMode);
                            if (ImGui::Combo("filpbookPlayMode", &v, items, IM_ARRAYSIZE(items)))
                                em.filpbookPlayMode = static_cast<FlipbookPlayMode>(v);
                        }

                        ImGui::DragFloat("holdTime", &em.holdTime, 0.05f, 0.0f, 99999.0f);

                        ImGui::TreePop();
                    }
                }

                // runtime info
                ImGui::Separator();
                ImGui::Text("runtime: particles=%d, elapsed=%.2f", (int)em.particles.size(), em.elapsed);
            }

            ImGui::PopID();
            ++i;
        }

        ImGui::Separator();
        ReadVariants(*fx);
        ImGui::PopID();
        return;
    }

    if (compName == "TextUI")
    {
        std::string fontPathKey = "ChooseTextUIFontDlgKey##" + std::to_string((uintptr_t)comp);

        for (auto& prop : t.get_properties())
        {
            rttr::variant value = prop.get_value(*comp);
            std::string name = prop.get_name().to_string();
            if (!value.is_valid()) continue;

            if (value.is_type<std::wstring>() && name == "fontPath")
            {
                // wstring -> utf8 (표시용)
                std::wstring curW = value.get_value<std::wstring>();
                std::string curPathUtf8 = WStringToUtf8(curW);

                ImGui::Text("Current Font Path: %s", curPathUtf8.c_str());

                if (ImGui::Button("Browse"))
                {
                    IGFD::FileDialogConfig config;
                    config.path = "../";
                    ImGuiFileDialog::Instance()->OpenDialog(fontPathKey, "Choose File", ".ttf,.ttc", config);
                }

                if (ImGuiFileDialog::Instance()->Display(fontPathKey))
                {
                    if (ImGuiFileDialog::Instance()->IsOk())
                    {
                        // 다이얼로그는 보통 UTF-8 std::string 반환
                        std::string filePathNameUtf8 = ImGuiFileDialog::Instance()->GetFilePathName();

                        // relative도 UTF-8 string 기준으로 처리
                        std::filesystem::path relativePath = std::filesystem::relative(std::filesystem::path(filePathNameUtf8));
                        std::string relativeUtf8 = relativePath.generic_string();

                        // utf8 -> wstring 저장
                        std::wstring relativeW = Utf8ToWString(relativeUtf8);
                        prop.set_value(*comp, relativeW);

                        // 기존 로직 유지: Decal이면 ChangeData 호출 (string 필요시 utf8 전달)
                        auto* textUI = dynamic_cast<TextUI*>(comp);
                        if (textUI) textUI->LoadFontAtlas(relativeW);
                    }
                    ImGuiFileDialog::Instance()->Close();
                }
            }
        }
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

    // GameObject AABB
    if (isGameObjectAABBDebugOpen)
    {
        SceneSystem::Instance().GetCurrentScene()->ForEachGameObject([&](GameObject* gameObject)
            {
                if (gameObject->IsDestory()) return;
                if (gameObject->GetComponent<FBXRenderer>() != nullptr) return;

                XMVECTOR color = XMVectorSet(0.0f, 1.0f, 0.0f, 1.0f);
                BoundingBox box = gameObject->GetAABB();
                DebugDraw::Draw(DebugDraw::g_Batch.get(), box, color);
            });
    }

    // Grid
    if (isGridDebugOpen)
    {
        RenderDebugGrid();
    }

    // Vision Ray
    RenderDebugVision();

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

void Editor::RenderDebugGrid()
{
    auto* grid = GridSystem::Instance().GetMainGrid();
    if (!grid) return;

    float defaultYThickness = 0.01f;
    float highlightYThickness = 10.0f; // 원점과 걸을 수 없는 그리드 두께

    int centerX = (grid->width - 1) / 2;
    int centerY = (grid->height - 1) / 2;


    // 중앙 기준 좌표: -centerX ~ +centerX-1, -centerY ~ +centerY-1
    for (int cy = -centerY; cy <= centerY; ++cy)
    {
        for (int cx = -centerX; cx <= centerX; ++cx)
        {
            GridCell* cell = grid->GetCellFromCenter(cx, cy);
            if (!cell) continue;

            // 중앙 기준 좌표 → 월드 위치
            Vector3 worldPos = grid->GridToWorldFromCenter(cx, cy);

            BoundingBox box;
            float halfSize = grid->cellSize * 0.5f;
            box.Center = XMFLOAT3(worldPos.x, worldPos.y, worldPos.z);

            float yThickness = defaultYThickness;
            XMVECTOR color = XMVectorSet(1.0f, 1.0f, 1.0f, 1.0f); // 기본 흰색
            bool drawCross = false; // X 표시 여부

            // 원점 (0,0) 중앙 그리드
            if (cx == 0 && cy == 0)
            {
                color = XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f); // 검은색
                yThickness = highlightYThickness;
            }
            // 걸을 수 없는 그리드
            else if (!cell->walkable)
            {
                color = XMVectorSet(1.0f, 0.0f, 0.0f, 1.0f); // 빨간색
                drawCross = true; // X 표시
            }

            box.Extents = XMFLOAT3(halfSize, yThickness, halfSize);

            // Draw: drawCross가 true면 X 표시 포함
            DebugDraw::Draw(DebugDraw::g_Batch.get(), box, color, drawCross);
        }
    }
}


void Editor::RenderDebugVision()
{
    SceneSystem::Instance().GetCurrentScene()->ForEachGameObject([&](GameObject* go)
        {
            if (auto* vision = go->GetComponent<VisionComponent>())
                vision->DrawDebugVision();
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

void Editor::LoadScene(HWND& hwnd)
{
    OPENFILENAMEA ofn = {};
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
        && !io.WantTextInput       // (선택) 텍스트 입력 중이면 차단
        && !ImGuizmo::IsOver()
        && !ImGuizmo::IsUsing();

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

    int propIndex = 0;
    // Get value from type
    for (auto& prop : t.get_properties())
    {
        std::string propId = prop.get_name().to_string();
        ImGui::PushID(propIndex);
        ImGui::PushID(propId.c_str());

        rttr::variant value = prop.get_value(inst);
        std::string name = prop.get_name().to_string();

        if (!value.is_valid())
            continue;

        // check metaData
        auto metaBool = prop.get_metadata(META_BOOL);
        auto metaBrowse = prop.get_metadata(META_BROWSE);
        auto metaInput = prop.get_metadata(META_INPUT);

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
        else if (value.is_type<std::string>() && metaInput.is_valid())
        {
            char buf[256]{};
            strncpy_s(buf, value.get_value<std::string>().c_str(), sizeof(buf) - 1);
            ImGui::InputText(name.c_str(), buf, sizeof(buf), ImGuiInputTextFlags_EnterReturnsTrue);
            prop.set_value(inst, std::string(buf));

        }
        else if (value.is_type<std::wstring>() && metaInput.is_valid())
        {
            std::string utf8 = WStringToUtf8(value.get_value<std::wstring>());

            char buf[256]{};
            strncpy_s(buf, utf8.c_str(), sizeof(buf) - 1);

            ImGui::InputText(name.c_str(), buf, sizeof(buf), ImGuiInputTextFlags_EnterReturnsTrue);

            prop.set_value(inst, Utf8ToWString(std::string(buf)));
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
        else if (value.is_type<DirectX::SimpleMath::Vector2>())
        {
            auto vec = value.get_value<DirectX::SimpleMath::Vector2>();
            if (ImGui::DragFloat2(name.c_str(), &vec.x, 0.1f))
                prop.set_value(inst, vec);
        }
        else if (value.is_type<DirectX::SimpleMath::Vector3>())
        {
            auto vec = value.get_value<DirectX::SimpleMath::Vector3>();
            if (ImGui::DragFloat3(name.c_str(), &vec.x, 0.1f))
                prop.set_value(inst, vec);
        }
        else if (value.is_type<DirectX::SimpleMath::Vector4>())
        {
            auto vec = value.get_value<DirectX::SimpleMath::Vector4>();
            if (ImGui::DragFloat4(name.c_str(), &vec.x, 0.1f))
                prop.set_value(inst, vec);
        }
        else if (value.is_type<Color>())
        {
            auto c = value.get_value<Color>();
            if (ImGui::ColorEdit3(name.c_str(), &c.x))
                prop.set_value(inst, c);
        }
        else if (value.is_type<string>() && !metaBrowse.is_valid())
        {
            std::string c = value.get_value<std::string>();

            if (ImGui::InputText(
                name.c_str(),
                c.data(),
                c.capacity() + 1,
                ImGuiInputTextFlags_CallbackResize,
                [](ImGuiInputTextCallbackData* data) -> int
                {
                    if (data->EventFlag == ImGuiInputTextFlags_CallbackResize)
                    {
                        auto* str = static_cast<std::string*>(data->UserData);
                        str->resize(data->BufTextLen);
                        data->Buf = str->data();
                    }
                    return 0;
                },
                &c))
            {
                prop.set_value(inst, c);
            }
        }
        else if (value.is_type<string>() && metaBrowse.is_valid())
        {
            std::string c = value.get_value<std::string>();

            // inst 포인터 (가능하면 실제 객체 포인터)
            void* p = inst.try_convert<void*>();
            std::uintptr_t instId = reinterpret_cast<std::uintptr_t>(p);

            // prop 이름을 섞어서 "같은 인스턴스의 다른 browse 프로퍼티"도 분리
            std::string propName = prop.get_name().to_string();

            // FileDialog key: inst + propName 조합 (표시용 텍스트는 ## 앞만, ID는 전체)
            std::string key = "ChooseFileNormalStringKey##" + std::to_string(instId) + "##" + propName;

            ImGui::Text("Current Path: %s", c.c_str());
            if (ImGui::Button("Browse"))
            {
                IGFD::FileDialogConfig config;
                config.path = "../";
                ImGuiFileDialog::Instance()->OpenDialog(key, "Choose File", ".png,.jpg,.fbx,.glb,.ttf,.ttc,.json", config);
            }
            if (ImGuiFileDialog::Instance()->Display(key))
            {
                if (ImGuiFileDialog::Instance()->IsOk())
                {
                    std::string filePathName = ImGuiFileDialog::Instance()->GetFilePathName();
                    std::filesystem::path relativePath = std::filesystem::relative(filePathName);
                    prop.set_value(inst, relativePath.string());
                }
                ImGuiFileDialog::Instance()->Close();
            }
        }
        else if (value.is_type<std::wstring>() && !metaBrowse.is_valid())
        {
            // wstring(UTF-16) -> UTF-8(string) : ImGui InputText 버퍼
            std::string c = WStringToUtf8(value.get_value<std::wstring>());

            if (ImGui::InputText(
                name.c_str(),
                c.data(),
                c.capacity() + 1,
                ImGuiInputTextFlags_CallbackResize,
                [](ImGuiInputTextCallbackData* data) -> int
                {
                    if (data->EventFlag == ImGuiInputTextFlags_CallbackResize)
                    {
                        auto* str = static_cast<std::string*>(data->UserData);
                        str->resize(data->BufTextLen);
                        data->Buf = str->data();
                    }
                    return 0;
                },
                &c))
            {
                // UTF-8(string) -> wstring(UTF-16)
                std::wstring w = Utf8ToWString(c);
                prop.set_value(inst, w);
            }
        }

        ImGui::PopID(); // propId
        ImGui::PopID(); // propIndex
        ++propIndex;
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

void Editor::RenderCameraPanel()
{
    if (!isCameraPanelOepn) return;

    ImGui::Begin("CameraPanel");
    {
        int curr = CameraSystem::Instance().GetCurrCameraIndex();
        int maxSize = CameraSystem::Instance().GetAllCamera().size();
        ImGui::SliderInt("index", &curr, 0, maxSize - 1);
        CameraSystem::Instance().SetCurrCamera(curr);
    }
    ImGui::End();
}

void Editor::RenderVariantDebugger()
{
    if (!isOpenVariantDebugger) return;
    ImGui::Begin("variant debugger");
    {
        float x = UIManager::Instance().GetSize().x;
        float y = UIManager::Instance().GetSize().y;
        string screenSizeStr = "ScreenSize : " + to_string(x) + ", " + to_string(y);
        ImGui::Text(screenSizeStr.c_str());

        float fps = 1.0f / GameTimer::Instance().UnscaledDeltaTime();
        string fpsStr = "Fps : " + to_string(fps);
        ImGui::Text(fpsStr.c_str());
    }
    ImGui::End();
}


void Editor::OnInputProcess(const Keyboard::State& KeyState, const Keyboard::KeyboardStateTracker& KeyTracker, const Mouse::State& MouseState, const Mouse::ButtonStateTracker& MouseTracker)
{
    isAABBPicking = false;

    if (MouseTracker.leftButton == Mouse::ButtonStateTracker::PRESSED)
    {
        if (!ImGui::GetIO().WantCaptureMouse && !ImGuizmo::IsOver() && !ImGuizmo::IsUsing())
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
