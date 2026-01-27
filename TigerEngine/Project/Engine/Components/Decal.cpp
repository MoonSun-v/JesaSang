#include "Decal.h"
#include "../EngineSystem/DecalSystem.h"
#include "../Manager/ShaderManager.h"
#include "../Components/Transform.h"
#include "../Util/JsonHelper.h"
#include <Windows.h>
#include <string>


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
        .property("DecalType",     &Decal::type)
        .property("Opacity",       &Decal::opacity)
        .property("UpThreshold",   &Decal::upThreshold)
        .property("Tiling",        &Decal::tiling)
        .property("Offset",        &Decal::offset)
        .property("TexturePath",   &Decal::decalTexturePath);
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
   
    if (decalTexturePath != "")
    {
        auto device = ShaderManager::Instance().device.Get();
        std::wstring wpath = ToWString(decalTexturePath);
        CreateTextureFromFile(device, wpath.c_str(), decalSRV.GetAddressOf(), TextureColorSpace::SRGB);
    }
}

void Decal::ChangeData(std::string path)
{
    decalTexturePath = path;

    if (decalTexturePath != "")
    {
        auto device = ShaderManager::Instance().device.Get();
        std::wstring wpath = ToWString(decalTexturePath);
        CreateTextureFromFile(device, wpath.c_str(), decalSRV.GetAddressOf(), TextureColorSpace::SRGB);
    }
}

std::wstring ToWString(const std::string& str)
{
    if (str.empty())
        return L"";

    int size_needed = MultiByteToWideChar(
        CP_UTF8,            // UTF-8
        0,
        str.c_str(),
        (int)str.size(),
        nullptr,
        0
    );

    std::wstring wstr(size_needed, 0);

    MultiByteToWideChar(
        CP_UTF8,
        0,
        str.c_str(),
        (int)str.size(),
        &wstr[0],
        size_needed
    );

    return wstr;
}