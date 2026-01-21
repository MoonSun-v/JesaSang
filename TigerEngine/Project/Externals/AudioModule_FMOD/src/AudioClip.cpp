#include "..\\include\\AudioClip.h"
#include <Windows.h>
#include "..\\fmod\\inc\\fmod_errors.h"

AudioClip::AudioClip(FMOD::System* system, const std::string& path, FMOD_MODE mode)
{
    if (!system) {
        return;
    }

    FMOD_RESULT result = system->createSound(path.c_str(), mode, nullptr, &m_Sound);
    if (result != FMOD_OK || !m_Sound) {
        OutputDebugStringA("[AudioClip] createSound failed: ");
        OutputDebugStringA(FMOD_ErrorString(result));
        OutputDebugStringA("\n");
        m_Sound = nullptr;
        return;
    }

    unsigned int lengthMs = 0;
    if (m_Sound->getLength(&lengthMs, FMOD_TIMEUNIT_MS) == FMOD_OK) {
        m_LengthSeconds = lengthMs / 1000.0f;
    }
}

AudioClip::~AudioClip()
{
    if (m_Sound) {
        m_Sound->release();
        m_Sound = nullptr;
    }
}
