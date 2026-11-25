// Offline sanity test of the DSP core (no JUCE needed).
// Build: g++ -std=c++17 -O2 -I stubs -I ../Source/DSP dsp_test.cpp ../Source/DSP/StutterProcessor.cpp ../Source/DSP/ReverseProcessor.cpp -o dsp_test
#include <cstdio>
#include <cmath>
#include <vector>
#include "AudioBufferManager.h"
#include "StutterProcessor.h"
#include "ReverseProcessor.h"

static const double SR = 48000.0;
static const int N = (int) (SR * 8);

static std::vector<float> makeVocal()
{
    std::vector<float> x (N);
    double ph = 0;
    for (int i = 0; i < N; ++i) {
        double t = i / SR;
        ph += 2 * M_PI * (180 + 60 * std::sin (2 * M_PI * 0.7 * t)) / SR;
        x[i] = (float) (0.5 * (0.5 + 0.5 * std::sin (2 * M_PI * 3 * t)) * std::sin (ph));
    }
    return x;
}

struct Stats { bool finite = true; float peak = 0, maxStep = 0, tailDiff = 0; };

static Stats analyse (const std::vector<float>& in, const std::vector<float>& out, int tailFrom)
{
    Stats s;
    for (int i = 0; i < N; ++i) {
        if (! std::isfinite (out[i])) s.finite = false;
        s.peak = std::max (s.peak, std::fabs (out[i]));
        if (i) s.maxStep = std::max (s.maxStep, std::fabs (out[i] - out[i - 1]));
        if (i >= tailFrom) s.tailDiff = std::max (s.tailDiff, std::fabs (out[i] - in[i]));
    }
    return s;
}

template <class F> static std::vector<float> run (const std::vector<float>& in, const int* sizes, int nSizes, F&& fn)
{
    std::vector<float> out (in), r (in);
    gz::AudioBufferManager hist; hist.prepare (SR, 10.0);
    int pos = 0, k = 0;
    while (pos < N) {
        int n = std::min (sizes[k++ % nSizes], N - pos);
        hist.write (out.data() + pos, r.data() + pos, n);
        fn (out.data() + pos, r.data() + pos, n, pos, hist);
        pos += n;
    }
    return out;
}

int main()
{
    auto in = makeVocal();
    const int sizes[] = { 64, 480, 1, 37, 1024, 333, 128 };   // awkward buffer sizes on purpose
    int fails = 0;
    auto check = [&] (const char* name, bool ok) { std::printf ("  [%s] %s\n", ok ? "OK " : "FAIL", name); if (! ok) ++fails; };

    // ---- STUTTER: capture at 1.0 s, 250 ms slice, +3 st, duration 1 s -> back to dry ----
    {
        gz::StutterProcessor st; st.prepare (SR);
        auto out = run (in, sizes, 7, [&] (float* l, float* r, int n, int pos, gz::AudioBufferManager& h) {
            gz::StutterProcessor::Params p;
            p.on = true; p.lengthSamples = (float) (0.25 * SR); p.pitchSemitones = 3.0f;
            p.durationSamples = SR; p.fadeMs = 8.0f;
            p.capture = pos >= (int) SR && pos < (int) (SR + 3000);
            st.process (l, r, n, p, h);
        });
        auto s = analyse (in, out, (int) (2.5 * SR));
        std::printf ("STUTTER: peak %.3f  maxStep %.4f  tailDiff %.6f\n", s.peak, s.maxStep, s.tailDiff);
        check ("finite & <= 1.0", s.finite && s.peak <= 1.0f);
        check ("no clicks (step < 0.05)", s.maxStep < 0.05f);
        check ("returns exactly to dry after duration", s.tailDiff == 0.0f);
    }
    // ---- STUTTER hold + recapture + off ----
    {
        gz::StutterProcessor st; st.prepare (SR);
        bool activeAt5 = false, activeAfterOff = true;
        auto out = run (in, sizes, 7, [&] (float* l, float* r, int n, int pos, gz::AudioBufferManager& h) {
            gz::StutterProcessor::Params p;
            p.on = pos < (int) (6 * SR); p.hold = true; p.lengthSamples = (float) (0.125 * SR); p.speed = 1.5f;
            p.capture = (pos >= (int) SR && pos < (int) (SR + 3000)) || (pos >= (int) (3 * SR) && pos < (int) (3 * SR + 3000));
            st.process (l, r, n, p, h);
            if (pos >= (int) (5 * SR) && pos < (int) (5 * SR + 480)) activeAt5 = st.isActive();
            if (pos >= (int) (7 * SR)) activeAfterOff = st.isActive();
        });
        auto s = analyse (in, out, (int) (6.2 * SR));
        std::printf ("HOLD/RECAPTURE: maxStep %.4f tailDiff %.6f\n", s.maxStep, s.tailDiff);
        check ("hold keeps running past duration", activeAt5);
        check ("no clicks incl. recapture", s.maxStep < 0.05f && s.finite);
        check ("OFF releases and returns to dry", ! activeAfterOff && s.tailDiff == 0.0f);
    }
    // ---- REVERSE transition: on at 2.0 s, window 600 ms ----
    {
        gz::ReverseProcessor rv; rv.prepare (SR);
        auto out = run (in, sizes, 7, [&] (float* l, float* r, int n, int pos, gz::AudioBufferManager& h) {
            gz::ReverseProcessor::Params p;
            p.on = pos >= (int) (2 * SR); p.mode = 1; p.windowSamples = (float) (0.6 * SR); p.transition = 0.3f;
            rv.process (l, r, n, p, h);
        });
        auto s = analyse (in, out, (int) (3.0 * SR));
        std::printf ("REVERSE TRANSITION: peak %.3f maxStep %.4f tailDiff %.6f\n", s.peak, s.maxStep, s.tailDiff);
        check ("finite & <= 1.0", s.finite && s.peak <= 1.0f);
        check ("no clicks", s.maxStep < 0.05f);
        check ("auto-returns to dry", s.tailDiff == 0.0f);
        // the reversed part must really differ from dry
        float diff = 0; for (int i = (int) (2.1 * SR); i < (int) (2.5 * SR); ++i) diff = std::max (diff, std::fabs (out[i] - in[i]));
        check ("reversed segment differs from dry", diff > 0.05f);
    }
    // ---- REVERSE loop mode with start/end/speed, on 2 s..4 s ----
    {
        gz::ReverseProcessor rv; rv.prepare (SR);
        auto out = run (in, sizes, 7, [&] (float* l, float* r, int n, int pos, gz::AudioBufferManager& h) {
            gz::ReverseProcessor::Params p;
            p.on = pos >= (int) (2 * SR) && pos < (int) (4 * SR); p.mode = 0; p.mix = 0.8f; p.volume = 1.2f;
            p.windowSamples = (float) (0.5 * SR); p.startPct = 20; p.endPct = 90; p.speed = 1.25f; p.fadeMs = 15;
            rv.process (l, r, n, p, h);
        });
        auto s = analyse (in, out, (int) (4.2 * SR));
        std::printf ("REVERSE LOOP: peak %.3f maxStep %.4f tailDiff %.6f\n", s.peak, s.maxStep, s.tailDiff);
        check ("finite", s.finite);
        check ("no clicks", s.maxStep < 0.06f);
        check ("OFF returns to dry", s.tailDiff == 0.0f);
    }
    std::printf (fails ? "\n%d CHECK(S) FAILED\n" : "\nALL CHECKS PASSED\n", fails);
    return fails != 0;
}
