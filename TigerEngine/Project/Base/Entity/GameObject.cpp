#include "GameObject.h"
#include "DirectXCollision.h"
#include "../System/ComponentFactory.h"
#include "../System/ScriptSystem.h"
#include "../System/RenderSystem.h"
#include "../System/InputSystem.h"

RTTR_REGISTRATION
{
    rttr::registration::class_<GameObject>("GameObject")
        .constructor<>()
            (rttr::policy::ctor::as_std_shared_ptr) 
        .property("Name", &GameObject::name);
}

std::string GameObject::GetName() const
{
    return name;
}

void GameObject::SetName(std::string str)
{
    name = str;
}

void GameObject::RemoveComponent(Component* comp)
{
    // 찾기
    for(auto it = components.begin(); it != components.end(); it++)
    {       
        if(*it == comp)
        {
            components.erase(it);
            break;
        }
    }   

    // handle 찾기 - 실제 객체 파괴하는 단계
    for (auto it = handles.begin(); it != handles.end(); it++)
    {
        auto objPtr = ObjectSystem::Instance().Get<Component>(*it);
        if (objPtr == comp)
        {
            if (auto renderComp = dynamic_cast<RenderComponent*>(objPtr))
            {
                RenderSystem::Instance().UnRegister(renderComp);
            }
            else
            {
                ScriptSystem::Instance().UnRegister(objPtr);
            }

            if (auto inputComp = dynamic_cast<InputProcesser*>(objPtr))
            {
                InputSystem::Instance().UnRegister(inputComp);
            }

            objPtr->OnDestory();
            ObjectSystem::Instance().Destory(*it);
            handles.erase(it);
            break;
        }
    }
}

Transform* GameObject::GetTransform() const
{
	return transform;
}

std::vector<Component*> GameObject::GetComponents()
{
    return components;
}

bool GameObject::IsDestory()
{
    return isDestory;
}

void GameObject::Destory()
{
	isDestory = true; // 가지고 있는 모든 컴포넌트 파괴하기
}

Scene *GameObject::GetScene()
{
    return currentScene;
}

void GameObject::SetScene(Scene* scene)
{
    currentScene = scene;
}

nlohmann::json GameObject::Serialize() const
{
    // gameObject
    //  properties
    //      name : gmaeName
    //      components 
    //          component_1
    //              component1_element_1
    //              component1_element_2
    //              ....
    //          component_2
    //              ....

    nlohmann::json datas;

    rttr::type t = rttr::type::get(*this);
    datas["type"] = t.get_name().to_string();    
    datas["properties"] = nlohmann::json::object(); // 객체 생성

    // 오브젝트 내용 직렬화화
    for(auto& prop : t.get_properties())
    {
        std::string propName = prop.get_name().to_string();
        rttr::variant value = prop.get_value(*this);

        if(value.is_type<std::string>())
        {
            datas["properties"][propName] = value.get_value<std::string>();
        }
    }

    // 컴포넌트 내용 직렬화
    datas["properties"]["components"] = nlohmann::json::object();
    auto& comps = datas["properties"]["components"];

    for (auto& comp : components)
    {
        comps[comp->GetName()] = comp->Serialize();
    }
    
    return datas;
}

void GameObject::Deserialize(const nlohmann::json objData)
{
    // objData : data["objects"]["properties"]

    rttr::type t = rttr::type::get(*this);
    if(!objData.contains("components")) return;

    const auto& registered = ComponentFactory::Instance().GetRegisteredComponents();

    for(auto& prop : objData["components"])
    {
        if(!prop.contains("type")) continue;

        std::string compName = prop["type"];
        if(compName == "Transform") // Transform은 게임 오브젝트가 생성 시에 추가된다.
        {
            auto trans = this->GetComponent<Transform>();
            trans->Deserialize(prop);
        }
        else    // 그 외 컴포넌트는 추가한다.
        {
            // 컴포넌트를 찾아서 factory에 등록되어있으면 컴포넌트 추가
            for (auto [name, create] : registered)
            {
                if (compName == name)
                {                   
                    auto createdComp = create(this);
                    createdComp->Deserialize(prop);

                    break;
                }
            }            
        }
    }
}

void GameObject::UpdateAABB()
{
    Transform* trans = transform;
    Vector3 updatedExtent = aabbBoxExtent * trans->GetScale();
    aabbBox.Center = trans->GetPosition() + aabbCenter;
    aabbBox.Extents = updatedExtent;
}

void GameObject::Initialize()
{
    aabbBoxExtent = { 10.0f, 10.0f, 10.0f };
    aabbBox = { {0.0f, 0.0f, 0.0f}, aabbBoxExtent };
    transform = AddComponent<Transform>();
}

void GameObject::SetAABB(BoundingBox aabb)
{
    aabbBox = aabb;
}

void GameObject::SetAABB(Vector3 min, Vector3 max, Vector3 centor)
{
    auto tran = transform;

    aabbBox.Center = tran->GetPosition();
    aabbBoxExtent = (max - min) / 2.0f;
    aabbCenter = centor;
}

void GameObject::ClearAll()
{
    for (auto it = components.begin(); it != components.end();)
    {
        it = components.erase(it);
    }

    // handle 찾기 - 실제 객체 파괴하는 단계
    for (auto it = handles.begin(); it != handles.end();)
    {
        auto objPtr = ObjectSystem::Instance().Get<Component>(*it);
        if (auto renderComp = dynamic_cast<RenderComponent*>(objPtr))
        {
            RenderSystem::Instance().UnRegister(renderComp);
        }
        else
        {
            ScriptSystem::Instance().UnRegister(objPtr);
        }

        if (auto inputComp = dynamic_cast<InputProcesser*>(objPtr))
        {
            InputSystem::Instance().UnRegister(inputComp);
        }

        it = handles.erase(it);
    }

    components.clear();
}
