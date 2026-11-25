#include "ReverseProcessor.h"

namespace gz
{
void ReverseProcessor::prepare (double sr)
{
    sampleRate = sr;
    maxSeg = (int) (sr * 8.0);
    seg.setSize (2, maxSeg + 4, false, true, false);
    gate.prepare (sr, 10.0);
    reset();
}

void ReverseProcessor::reset() noexcept
{
    playing = lastOn = false;
    segLen = 0; n = 0.0; playLen = 1.0;
    gate.reset (0.0f);
}

void ReverseProcessor::trigger (const Params& p, const AudioBufferManager& hist) noexcept
{
    const int window = std::max (256, std::min ((int) p.windowSamples, std::min (maxSeg, hist.maxRegion())));

    float s = std::min (1.0f, std::max (0.0f, p.startPct * 0.01f));
    float e = std::min (1.0f, std::max (0.0f, p.endPct * 0.01f));
    if (e - s < 0.02f) { e = std::min (1.0f, s + 0.02f); s = e - 0.02f; }   // keep a sane minimum

    int len = (int) ((e - s) * (float) window);
    len = std::max (64, std::min (len, window));
    // The region starts 'ago' samples back and must end at or before "now": len <= ago.
    const int ago = std::max (len, std::min ((int) ((1.0f - s) * (float) window), hist.maxRegion()));

    for (int ch = 0; ch < 2; ++ch)
        hist.copyRegion (ch, ago, len, seg.getWritePointer (ch));

    segLen = len;
    playLen = (double) len / std::max (0.1, (double) p.speed);
    n = 0.0;
    playing = true;
}

void ReverseProcessor::process (float* l, float* r, int numSamples, const Params& p,
                                const AudioBufferManager& hist) noexcept
{
    const bool edge = p.on && ! lastOn;
    lastOn = p.on;
    // Re-triggering within ~10 ms of switching off can cause a tiny step; acceptable corner case.
    if (edge && hist.maxRegion() >= 256)
        trigger (p, hist);

    if (! playing) return;

    for (int i = 0; i < numSamples; ++i)
    {
        gate.setTarget (p.on ? 1.0f : 0.0f);
        const float gv = gate.next();
        if (! p.on && gv <= 0.0f) { playing = false; break; }

        const float* sl = seg.getReadPointer (0);
        const float* sr2 = seg.getReadPointer (1);

        // ---- envelope of the reversed signal ----
        const float fadeS = (float) std::min (std::max (1.0, (double) p.fadeMs * 0.001 * sampleRate), 0.5 * playLen);
        double tailS = fadeS;
        if (p.mode == 1) tailS = std::max (tailS, (double) p.transition * 0.5 * playLen);
        tailS = std::min (tailS, 0.5 * playLen);

        float env = smoothstep ((float) n / fadeS) * smoothstep ((float) ((playLen - n) / tailS));
        if (p.mode == 1)
        {
            const float t = (float) (n / playLen);
            env *= t * t;               // swell: reversed vocal "sucks in" towards the downbeat
        }

        // ---- read backwards with linear interpolation ----
        double idx = (double) (segLen - 1) - n * (double) p.speed;
        if (idx < 0.0) idx = 0.0;
        const int i0 = (int) idx;
        const int i1 = std::min (i0 + 1, segLen - 1);
        const float fr = (float) (idx - (double) i0);
        const float wl = (sl[i0] * (1.0f - fr) + sl[i1] * fr) * p.volume;
        const float wr = (sr2[i0] * (1.0f - fr) + sr2[i1] * fr) * p.volume;

        const float m = p.mix * env * smoothstep (gv);
        l[i] = l[i] * (1.0f - m) + wl * m;
        r[i] = r[i] * (1.0f - m) + wr * m;

        n += 1.0;
        if (n >= playLen)
        {
            if (p.mode == 1 && p.autoReturn) { playing = false; break; }   // env == 0 here: seamless
            if (p.on) trigger (p, hist);   // loop: grab the next fragment (env ~ 0 at the seam)
            else { playing = false; break; }
        }
    }
}
} // namespace gz
