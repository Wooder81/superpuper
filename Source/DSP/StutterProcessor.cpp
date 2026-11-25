#include "StutterProcessor.h"

namespace gz
{
void StutterProcessor::prepare (double sr)
{
    sampleRate = sr;
    maxLoop = (int) (sr * 4.0);                 // up to 4 s loop
    for (auto& b : bufs) b.setSize (2, maxLoop + 4, false, true, false);
    gate.prepare (sr, 10.0);
    reset();
}

void StutterProcessor::reset() noexcept
{
    active = pending = lastCapture = false;
    cur = 0; lens[0] = lens[1] = 0;
    readPos = elapsed = 0.0;
    gate.reset (0.0f);
}

void StutterProcessor::capture (const Params& p, const AudioBufferManager& hist) noexcept
{
    int len = (int) std::lround (p.lengthSamples / std::max (0.05f, p.speed));
    len = std::max (64, std::min (len, std::min (maxLoop, hist.maxRegion())));

    // While running, write into the idle buffer; it is swapped in at the next loop seam
    // (where the fade makes the signal ~0), so re-capturing never clicks.
    const int target = active ? 1 - cur : cur;
    for (int ch = 0; ch < 2; ++ch)
        hist.copyRegion (ch, len, len, bufs[target].getWritePointer (ch));
    lens[target] = len;
    elapsed = 0.0;

    if (active) pending = true;
    else { cur = target; readPos = 0.0; active = true; }
}

void StutterProcessor::process (float* l, float* r, int n, const Params& p,
                                const AudioBufferManager& hist) noexcept
{
    const bool edge = p.capture && ! lastCapture;      // momentary button: act on rising edge
    lastCapture = p.capture;
    if (edge && p.on && hist.maxRegion() >= 64)
        capture (p, hist);

    if (! active) return;

    const double rate = std::pow (2.0, (double) p.pitchSemitones / 12.0);
    const float fadeSamples = (float) std::max (1.0, (double) p.fadeMs * 0.001 * sampleRate);

    for (int i = 0; i < n; ++i)
    {
        const bool run = p.on && (p.hold || elapsed < p.durationSamples);
        gate.setTarget (run ? 1.0f : 0.0f);
        const float gv = gate.next();
        if (! run && gv <= 0.0f) { active = false; pending = false; break; }

        const int len = lens[cur];
        const float* sl = bufs[cur].getReadPointer (0);
        const float* sr2 = bufs[cur].getReadPointer (1);

        // Per-repeat envelope: fade in at the start, fade out at the end of the slice.
        const float fs = std::min (fadeSamples, 0.5f * (float) len);
        const float pos = (float) readPos;
        const float g = std::max (0.0f, std::min ({ 1.0f, pos / fs, ((float) (len - 1) - pos) / fs }));

        const int i0 = (int) readPos;
        const int i1 = std::min (i0 + 1, len - 1);
        const float fr = (float) (readPos - (double) i0);
        const float wl = (sl[i0] * (1.0f - fr) + sl[i1] * fr) * g;
        const float wr = (sr2[i0] * (1.0f - fr) + sr2[i1] * fr) * g;

        const float m = p.mix * smoothstep (gv);
        l[i] = l[i] * (1.0f - m) + wl * m;
        r[i] = r[i] * (1.0f - m) + wr * m;

        readPos += rate;
        elapsed += 1.0;
        if (readPos >= (double) len)
        {
            readPos -= (double) len;
            if (readPos >= (double) len) readPos = 0.0;
            if (pending) { cur = 1 - cur; pending = false; readPos = 0.0; }
        }
    }
}
} // namespace gz
