// 초보용 예제: FMOD 오디오 사용 흐름
#include "AudioSystem.h"
#include "AudioListenerComponent.h"
#include "AudioSourceComponent.h"
#include "AudioClip.h"
#include <directxmath.h>
#include <memory>

// 전역 오디오 객체들
AudioSystem g_Audio;
AudioListenerComponent g_Listener;
AudioSourceComponent g_Source;
std::shared_ptr<AudioClip> g_Clip;

// 리스너(카메라)와 소스(오브젝트) 좌표 저장용
DirectX::XMFLOAT3 g_ListenerPos{ 0.0f, 0.0f, 0.0f };
DirectX::XMFLOAT3 g_ListenerVel{ 0.0f, 0.0f, 0.0f };
DirectX::XMFLOAT3 g_ListenerFwd{ 0.0f, 0.0f, 1.0f };
DirectX::XMFLOAT3 g_ListenerUp{ 0.0f, 1.0f, 0.0f };
DirectX::XMFLOAT3 g_SourcePos{ 0.0f, 0.0f, 0.0f };
DirectX::XMFLOAT3 g_SourceVel{ 0.0f, 0.0f, 0.0f };

void Audio_Init()
{
    // 1) FMOD 시스템 초기화
    g_Audio.Init();
    g_Audio.Set3DSettings(1.0f, 1.0f, 1.0f);

    // 2) 리스너/소스 컴포넌트 초기화
    g_Listener.Init(&g_Audio);
    g_Source.Init(&g_Audio);

    // 3) 리스너 바인딩(카메라 좌표)
    AudioTransformRef listenerRef{};
    listenerRef.position = &g_ListenerPos;
    listenerRef.velocity = &g_ListenerVel;
    listenerRef.forward = &g_ListenerFwd;
    listenerRef.up = &g_ListenerUp;
    g_Listener.BindTransform(listenerRef);

    // 4) 소스 바인딩(오브젝트 좌표)
    AudioTransformRef sourceRef{};
    sourceRef.position = &g_SourcePos;
    sourceRef.velocity = &g_SourceVel;
    g_Source.BindTransform(sourceRef);

    // 5) 사운드 파일 로드 (.wav/.mp3)
    g_Clip = g_Audio.CreateClip("..\\Assets\\Audio\\SFX\\test.wav", FMOD_3D);
    // 파일 경로는 프로젝트 구조에 맞게 수정
    if (g_Clip) {
        g_Source.SetClip(g_Clip);
        g_Source.SetLoop(true);
        g_Source.SetVolume(0.7f);
        g_Source.Set3DMinMaxDistance(1.0f, 200.0f);
        g_Source.Play();
    }
}

void Audio_Update()
{
    // 매 프레임 카메라/오브젝트 위치를 갱신해야 함
    // 예: g_ListenerPos = 카메라 위치; g_SourcePos = 오브젝트 위치;
    // 필요하면 속도도 계산해서 g_ListenerVel / g_SourceVel에 넣기

    // 1) 리스너 업데이트
    g_Listener.Update();
    // 2) 소스 위치 업데이트
    g_Source.Update3D();
    // 3) FMOD 시스템 업데이트
    g_Audio.Update();
}

void Audio_Shutdown()
{
    // 종료 시 FMOD 해제
    g_Audio.Shutdown();
}
