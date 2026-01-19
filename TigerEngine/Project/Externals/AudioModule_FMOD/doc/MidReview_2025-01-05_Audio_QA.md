# Q&A - Audio (2025-01-05)

Q: Why FMOD and not XAudio2?
A: FMOD gives quick 3D spatial audio with fewer low-level details; faster to integrate for the milestone.

Q: Does it require engine core changes?
A: No. It is an external module and integrates via GameApp OnInitialize/OnUpdate only.

Q: How do we attach audio to objects?
A: Bind object XMFLOAT3 position/velocity to AudioSourceComponent.

Q: Is listener always the camera?
A: Usually yes; it binds to camera position/forward/up each frame.

Q: Performance risk?
A: Low for current scope; only a few sources. Can scale with pooling later.

Q: Next steps after mid review?
A: Provide example scene, add pooling and distance-based priority if needed.
