#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include <cmath>

namespace gz
{
/** Stereo circular "history" buffer that always records the incoming vocal.
    Memory is allocated only in prepare() (never on the audio thread).
    Stutter and Reverse "capture" by copying a region of recent history. */
class AudioBufferManager
{
public:
    void prepare (double sampleRate, double seconds)
    {
        size = (int) std::ceil (sampleRate * seconds) + 8;
        data.setSize (2, size, false, true, false);
        data.clear();
        writePos = 0;
    }

    void clear() noexcept { data.clear(); writePos = 0; }

    /** Largest 'ago' / 'len' that copyRegion() accepts. */
    int maxRegion() const noexcept { return size - 4; }

    void write (const float* l, const float* r, int n) noexcept
    {
        float* dl = data.getWritePointer (0);
        float* dr = data.getWritePointer (1);
        int w = writePos;
        for (int i = 0; i < n; ++i)
        {
            dl[w] = l[i];
            dr[w] = r[i];
            if (++w >= size) w = 0;
        }
        writePos = w;
    }

    /** Copies 'len' samples that START 'ago' samples before the write head.
        Requires len <= ago <= maxRegion(). */
    void copyRegion (int channel, int ago, int len, float* dest) const noexcept
    {
        const float* src = data.getReadPointer (channel);
        int idx = writePos - ago;
        if (idx < 0) idx += size;
        for (int i = 0; i < len; ++i)
        {
            dest[i] = src[idx];
            if (++idx >= size) idx = 0;
        }
    }

private:
    juce::AudioBuffer<float> data;
    int size = 0, writePos = 0;
};
} // namespace gz
