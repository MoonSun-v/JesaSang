# FMOD 오디오 모듈 사용법 (쉽게)

## 1) 준비물
- FMOD 라이브러리 경로 설정 (include/lib)
- fmod.dll 또는 fmodL.dll을 출력 폴더에 복사

## 2) 초기화 (한 번만)
- AudioSystem.Init() 호출
- Set3DSettings(1, 1, 1)로 3D 세팅

## 3) 리스너 연결 (카메라)
- 카메라 위치/방향 XMFLOAT3를 리스너에 바인딩
- Listener/Source 컴포넌트를 Init(&AudioSystem)

## 4) 오디오 클립 만들기
- CreateClip("파일경로", FMOD_3D) 호출
- 경로는 실행 파일 기준 상대경로 사용 가능

## 5) 오디오 소스에 클립 연결
- Source.SetClip(clip)
- Source.SetLoop(true) / SetVolume / SetPitch
- Source.Set3DMinMaxDistance(1, 200)
- Source.Play() 또는 PlayOneShot()

## 6) 매 프레임 업데이트
- Listener.Update() 먼저
- Source.Update3D() 다음
- 마지막에 AudioSystem.Update()

## 7) 종료
- AudioSystem.Shutdown() 호출

## 8) 파일 포맷 (.mp3/.wav)
- FMOD Core API는 .mp3와 .wav 둘 다 사용 가능
- 파일이 손상되었거나 코덱이 특수한 경우 로드 실패 가능
- 실패 시 WAV(PCM)로 변환하면 가장 안전
