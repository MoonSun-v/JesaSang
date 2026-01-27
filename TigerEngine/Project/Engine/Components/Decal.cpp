#include "Decal.h"
#include "../EngineSystem/DecalSystem.h"
#include "../Manager/ShaderManager.h"
#include "../Components/Transform.h"
#include "../Util/JsonHelper.h"


RTTR_REGISTRATION
{
    rttr::registration::enumeration<DecalType>("DecalType")
        (
            rttr::value("TextureMap", DecalType::TextureMap),
            rttr::value("RingEffect", DecalType::RingEffect)
        );

    rttr::registration::class_<Decal>("Decal")
        .constructor<>()
            (rttr::policy::ctor::as_std_shared_ptr)
        .property("DecalType",   &Decal::type)
        .property("TexturePath", &Decal::decalTexturePath);
}

void Decal::OnInitialize()
{
    DecalSystem::Instance().Register(this);
}

void Decal::OnDestory()
{
    DecalSystem::Instance().UnRegister(this);
}

nlohmann::json Decal::Serialize()
{
    return JsonHelper::MakeSaveData(this);
}

void Decal::Deserialize(nlohmann::json data)
{
    JsonHelper::SetDataFromJson(this, data);
    
    // TODO :: Texture SRV Create
    if (decalTexturePath != "")
    {
        auto device = ShaderManager::Instance().device.Get();
        //CreateTextureFromFile(device, decalTexturePath, decalSRV.GetAddressOf(), TextureColorSpace::SRGB);
    }
}

void Decal::ChangeData(std::string path)
{
    decalTexturePath = path;

    // TODO :: Texture SRV Create
    if (decalTexturePath != "")
    {
        auto device = ShaderManager::Instance().device.Get();
        //CreateTextureFromFile(device, decalTexturePath, decalSRV.GetAddressOf(), TextureColorSpace::SRGB);
    }
}