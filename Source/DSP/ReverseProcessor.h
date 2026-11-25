#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include "AudioBufferManager.h"
#include "DspUtils.h"

namespace gz
{
/** Vocal reverse.
    Turning REVERSE on takes the last 'window' samples of vocal from the history
    (optionally just the Start..End part of it), and plays them backwards.
      mode 0 "Reverse"    : reversed fragments repeat while ON (reverse-delay style)
      mode 1 "Transition" : one reversed swell (gain grows towards the end), then it
                            cross-fades back into the live vocal (auto return).
    The plugin can only reverse audio it has already heard (no look-ahead), so the
    reversed fragment is the vocal that came BEFORE the moment you switch it on. */
class ReverseProcessor
{
public:
    struct Params
    {
        bool  on = false;
        int   mode = 1;                 // 0 = Reverse, 1 = Transition
        float windowSamples = 24000.0f; // how far back we look
        float startPct = 0.0f, endPct = 100.0f; // part of the window to reverse
        float mix = 1.0f;               // 0..1
        float volume = 1.0f;            // linear gain of the reversed signal
        float fadeMs = 20.0f;
        float speed = 1.0f;             // playback speed (varispeed)
        float transition = 0.3f;        // 0..1: size of the cross-fade back to the dry vocal
        bool  autoReturn = true;        // Transition mode: go back to dry after one pass
    };

    void prepare (double sampleRate);
    void reset() noexcept;
    void process (float* l, float* r, int n, const Params& p, const AudioBufferManager& hist) noexcept;
    bool isActive() const noexcept { return playing; }

private:
    void trigger (const Params& p, const AudioBufferManager& hist) noexcept;

    double sampleRate = 44100.0;
    int maxSeg = 0;
    juce::AudioBuffer<float> seg;
    int segLen = 0;
    double n = 0.0, playLen = 1.0;
    bool playing = false, lastOn = false;
    Ramp gate;
};
} // namespace gz
