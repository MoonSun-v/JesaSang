struct PostProcessWorldData
{
    // Base
    bool    isHDR = 0;               // LDR/HDR
    bool    useDefaultGamma = 1;     // Linear -> SRGB
    float   defaultGamma = 2.2f;     // Gamma (defalut)
    float   exposure = 0;            // 노출

    // Enable
    bool useColorAdjustments = 0;
    bool useWhiteBalance = 0;
    bool useLGG = 0;
    bool useVignette = 0;
    bool useFilmGrain = 0;
    bool useBloom = 0;

    // Color Adjustments (대비, 채도, Hue Shift, Tint)
    float   contrast = 1;
    float   saturation = 1;
    bool    useHueShift = 0;
    float   hueShift = 0;
    bool    useColorTint = 0;
    Vector3 colorTint = { 1,1,1 };
    float   colorTint_strength = 0.5;

    // White Balance (온도, 색조)
    float temperature = 0;
    float tint = 0;

    // Lift / Gamma / Gain (어두운톤, 미드톤, 밝은톤 밝기 조정)
    bool  useLift = 0;
    bool  useGamma = 0;
    bool  useGain = 0;

    Vector3 lift = { 0,0,0 };
    float   lift_strength = 0.5;
    Vector3 gamma = { 0,0,0 };
    float   gamma_strength = 1.0;
    Vector3 gain = { 0,0,0 };
    float   gain_strength = 0.5;

    // Vinette
    float   vignette_intensity = 0.5;
    float   vignette_smoothness = 0.5;
    Vector2 vignetteCenter = { 0.5,0.5 };
    Vector3 vignetteColor = { 0,0,0 };

    // FilmGrain
    float grain_intensity = 0.2;
    float grain_response = 0.8;
    float grain_scale = 1;

    // Bloom
    float bloom_threshold = 1.0f;
    float bloom_intensity = 0.8f;
    float bloom_scatter = 0.5f;
    float bloom_clamp = 0.0f;
    Vector3 bloom_tint = { 1.0f, 1.0f, 1.0f };
    int     srcMip = 0;                      // SampleLevel용 mip 인덱스
    Vector2 srcTexelSize = { 0.0f, 0.0f };   // 패스에서 읽고 있는 mip 레벨의 텍스처 해상도를 기준으로 한 texel size
};


struct FrameWorldData
{
    float   time;
    float   deltaTime;
    Vector2 screenSize;
    Vector2 shadowMapSize = { 8192,8192 };
    Vector3 cameraPos;
}