# FMOD Audio Module (Drop-in)

This folder is a standalone FMOD-based audio module extracted from your Sound_Test reference.
It is designed to be added to any C++ DX11 app without editing engine core files.

## 1) FMOD include/lib settings (Visual Studio)

- Include path:
  - C:\Program Files (x86)\FMOD SoundSystem\FMOD Studio API Windows\api\core\inc
- Library path:
  - C:\Program Files (x86)\FMOD SoundSystem\FMOD Studio API Windows\api\core\lib\x64
- Link libraries:
  - Debug: fmodL_vc.lib
  - Release: fmod_vc.lib
- Runtime DLL:
  - Copy fmodL.dll (Debug) or fmod.dll (Release) to your output folder.

Tip: add a Post-Build step to copy the DLL:
```
xcopy /Y /D "C:\Program Files (x86)\FMOD SoundSystem\FMOD Studio API Windows\api\core\lib\x64\fmod.dll" "$(OutDir)"
```

## 2) Add module files to your app project

Add the following files to your project (or build them into a static library):
- include\AudioSystem.h, src\AudioSystem.cpp
- include\AudioClip.h, src\AudioClip.cpp
- include\AudioListener.h, src\AudioListener.cpp
- include\AudioSource.h, src\AudioSource.cpp
- include\AudioTransform.h
- include\AudioListenerComponent.h, src\AudioListenerComponent.cpp
- include\AudioSourceComponent.h, src\AudioSourceComponent.cpp

## 3) Minimal usage pattern (works with GameApp)

```cpp
AudioSystem g_Audio;
AudioListenerComponent g_Listener;
AudioSourceComponent g_Source;
std::shared_ptr<AudioClip> g_Clip;
DirectX::XMFLOAT3 g_ListenerPos{};
DirectX::XMFLOAT3 g_ListenerVel{};
DirectX::XMFLOAT3 g_ListenerFwd{ 0.0f, 0.0f, 1.0f };
DirectX::XMFLOAT3 g_ListenerUp{ 0.0f, 1.0f, 0.0f };

// OnInitialize:
g_Audio.Init();
g_Audio.Set3DSettings(1.0f, 1.0f, 1.0f);
g_Listener.Init(&g_Audio);
g_Source.Init(&g_Audio);

// Bind to persistent data (example: FreeCamera in GameApp)
AudioTransformRef listenerRef{};
listenerRef.position = &g_ListenerPos;
listenerRef.velocity = &g_ListenerVel;
listenerRef.forward = &g_ListenerFwd;
listenerRef.up = &g_ListenerUp;
g_Listener.BindTransform(listenerRef);

// Create a 3D clip
g_Clip = g_Audio.CreateClip("..\\Assets\\Audio\\SFX\\sfx_test_loop.wav", FMOD_3D);
g_Source.SetClip(g_Clip);
g_Source.SetLoop(true);
g_Source.SetVolume(0.7f);
g_Source.Set3DMinMaxDistance(1.0f, 200.0f);
g_Source.Play();

// In your per-frame update:
g_ListenerPos = { m_Camera.m_Position.x, m_Camera.m_Position.y, m_Camera.m_Position.z };
auto fwd = m_Camera.GetForward();
g_ListenerFwd = { fwd.x, fwd.y, fwd.z };
g_Listener.Update();
g_Source.Update3D();
g_Audio.Update();

// On shutdown:
g_Audio.Shutdown();
```

## 4) Attaching to any object (component-style)

For any object that stores position/velocity as DirectX::XMFLOAT3, bind it:
```cpp
AudioTransformRef srcRef{};
srcRef.position = &objectPos;
srcRef.velocity = &objectVel;
g_Source.BindTransform(srcRef);
```

Now the FMOD 3D position follows your object without needing engine changes.

## 5) Common pitfalls

- Always call g_Audio.Update() every frame.
- Use FMOD_3D for spatial sources and set min/max distance.
- Ensure the listener is updated before sources each frame.
- If you hear nothing: check DLL copy, volume, and that the clip path is correct.
