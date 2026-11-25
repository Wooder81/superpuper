#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include "AudioBufferManager.h"
#include "DspUtils.h"

namespace gz
{
/** Vocal stutter / beat repeat.
    CAPTURE copies the latest 'length' samples from the history and loops them.
    Every repeat has a fade-in/out (no seam clicks); engaging/releasing the
    effect goes through a 10 ms gate (no clicks). Pitch is tape-style
    (varispeed): it also changes the repeat period. */
class StutterProcessor
{
public:
    struct Params
    {
        bool  on = false, capture = false, hold = false;
        float lengthSamples = 11025.0f;  // slice length before 'speed' is applied
        float speed = 1.0f;              // repeat-rate multiplier (loop = length / speed)
        float pitchSemitones = 0.0f;
        float mix = 1.0f;                // 0..1
        float fadeMs = 8.0f;             // fade at both ends of every repeat
        double durationSamples = 48000.0; // auto-return after this (ignored when hold)
    };

    void prepare (double sampleRate);
    void reset() noexcept;
    void process (float* l, float* r, int n, const Params& p, const AudioBufferManager& hist) noexcept;
    bool isActive() const noexcept { return active; }

private:
    void capture (const Params& p, const AudioBufferManager& hist) noexcept;

    double sampleRate = 44100.0;
    int maxLoop = 0;
    juce::AudioBuffer<float> bufs[2];   // double buffer: a new capture swaps in at a loop seam
    int lens[2] { 0, 0 };
    int cur = 0;
    bool pending = false, active = false, lastCapture = false;
    double readPos = 0.0, elapsed = 0.0;
    Ramp gate;
};
} // namespace gz
