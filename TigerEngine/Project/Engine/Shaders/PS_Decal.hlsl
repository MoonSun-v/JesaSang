// [ Decal Pixel Shader ]
// Decal Volume Box의 Local UV 좌표를 기반으로 Decal Texture를 샘플링하여 출력합니다.
// - Texture mapping
// - Ring Effect (TODO)

#include <shared.fxh>

// --- Texture Bind Slot ------------------
Texture2D normalTex : register(t7);
Texture2D depthTex : register(t10);
Texture2D decalTex : register(t19);

// --- Sampler Bind Slot ------------------
SamplerState samLinear : register(s0);
SamplerState samLinearClamp : register(s2);


// 링이 끝(바깥)으로 갈수록 투명해지는 정도 조절 파라미터
static const float endFadeStart01 = 0.7f;
static const float endFadeFeather01 = 0.12f;   // 0.05~0.2

float RingMask(float2 uv, float t01)
{
    // uv: [0,1], center: (0.5,0.5)
    float2 p = (uv - 0.5f) * 2.0f; // [-1,1]
    float r = length(p); // 0 ~ sqrt(2)

    // 박스 중심 -> 코너까지의 최대 원형 반경
    const float maxBoxRadius = 1.41421356f; // sqrt(2)

    // ringMaxRadius를 "비율"로 해석
    // 1.0 = 박스 끝(코너)까지
    // 0.5 = 그 절반까지만
    float radiusLimit = ringMaxRadius * maxBoxRadius;

    // 현재 링 반경
    float radius = t01 * radiusLimit;

    // 링 마스크
    float d = abs(r - radius);
    float ring = 1.0f - smoothstep(ringThickness, ringThickness + ringFeather, d);

    // 최대 진행 가능 반경 바깥은 페이드
    float outer = 1.0f - smoothstep(radiusLimit, radiusLimit + ringFeather, r);

    // 링 진행이 끝에 가까워질수록 페이드
    float endFade = 1.0f - smoothstep(
        endFadeStart01,
        endFadeStart01 + endFadeFeather01,
        t01
    );

    return ring * outer * endFade;
}

float RingMaskContinuous(float2 uv, float base01)
{
    // 링 개수
    int RING_COUNT = 5;
    if (ringMaxRadius <= 0.6)
    {
        RING_COUNT = 3;
    }
    float m = 0.0f;

    [unroll]
    for (int i = 0; i < RING_COUNT; ++i)
    {
        float phase = (float) i / (float) RING_COUNT;
        m += RingMask(uv, frac(base01 + phase));
    }

    return saturate(m);
}

float RingMaskContinuous_SpawnLimited(float2 uv, float tRaw)
{
    // 링 개수
    int RING_COUNT = 5;
    if (ringMaxRadius <= 0.6)
    {
        RING_COUNT = 3;
    }
    float m = 0.0f;

    float maxR = max(ringMaxRadius, 0.0001f);
    float spd = max(ringSpeed, 0.0001f);

    // 한 사이클 시간(반경이 maxR만큼 커지는 시간)
    float Tcycle = maxR / spd;

    // 정규화 진행량 (1 증가 = 한 사이클)
    float u = tRaw / Tcycle;

    // 링이 막 태어날 때 살짝 부드럽게
    const float spawnFadeInTime = 0.12f;

    [unroll]
    for (int i = 0; i < RING_COUNT; ++i)
    {
        float phase = (float) i / (float) RING_COUNT; // 0, 0.2, 0.4, 0.6, 0.8

        // u - phase 로 바꿔서, phase가 큰 링은 늦게 스폰되게
        float x = u - phase;

        // 스폰 전이면 표시하지 않음 (처음에 5개가 한번에 뜨는 현상 제거)
        float started = step(0.0f, x);

        float cycleIdx = floor(x);  // 0,1,2... (x>=0일 때만 의미 있음)
        float t01 = frac(x);        // 사이클 내 진행(0~1)

        // 이 링이 생성된 절대시간
        float spawnTime = (cycleIdx + phase) * Tcycle;

        // duration 이후 생성되는 링은 컷
        float alive = step(spawnTime, ringDuration);

        // 생성 직후 페이드 인
        float ringAge = tRaw - spawnTime;
        float birthFade = smoothstep(0.0f, spawnFadeInTime, ringAge);

        m += RingMask(uv, t01) * started * alive * birthFade;
    }

    return saturate(m);
}

float4 main(PS_Position_INPUT input) : SV_TARGET
{
    float2 uvScreen = input.pos.xy / screenSize;
    float depth = depthTex.Sample(samLinearClamp, uvScreen).r;

    float4 clip;
    clip.x = uvScreen.x * 2.0f - 1.0f;
    clip.y = (1.0f - uvScreen.y) * 2.0f - 1.0f;
    clip.z = depth;
    clip.w = 1.0f;

    float4 worldH = mul(clip, invViewProjection);
    float3 worldPos = worldH.xyz / worldH.w;

    float3 local = mul(float4(worldPos, 1.0f), decalInvWorld).xyz;

    const float h = 10.0f;
    if (any(abs(local) > h))
        discard;

    float2 uvDecal = (local.xz / (2.0f * h)) + 0.5f;
    uvDecal = uvDecal * tiling + offset;

    // Decal Type
    if (decalType == 0) // TextureMap
    {
        float4 c = decalTex.Sample(samLinearClamp, uvDecal);
        c.a *= opacity;
        if (c.a <= 0.001f)
            discard;
        return c;
    }
    else // RingEffect
    {
        float tRaw = time - ringStartTime;
        if (tRaw < 0.0f)
            discard;

        // 링은 계속 이동
        float maxR = max(ringMaxRadius, 0.0001f);
        float spd = max(ringSpeed, 0.0001f);

        // 루프 경계 페이드
        float base01 = frac((tRaw * spd) / maxR);
        float cycleFade = smoothstep(0.0f, 0.05f, base01) * (1.0f - smoothstep(0.95f, 1.0f, base01));

        // spawn 제한 + 링 이동
        float mask = RingMaskContinuous_SpawnLimited(uvDecal, tRaw);

        float a = mask * cycleFade * opacity;
        if (a <= 0.001f)
            discard;

        return float4(ringColor, a);
    }
}