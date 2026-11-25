#pragma once
#include <algorithm>
#include <cmath>

namespace gz
{
inline float dbToGain (float db) noexcept { return std::pow (10.0f, db * 0.05f); }

// Soft clipper: perfectly transparent below 0.9, smooth tanh saturation above,
// output can never exceed 1.0. Protects the output stage from clipping.
inline float softClip (float x) noexcept
{
    constexpr float knee = 0.9f;
    const float a = std::fabs (x);
    if (a <= knee) return x;
    const float y = knee + (1.0f - knee) * std::tanh ((a - knee) / (1.0f - knee));
    return x < 0.0f ? -y : y;
}

inline float smoothstep (float x) noexcept
{
    x = std::min (1.0f, std::max (0.0f, x));
    return x * x * (3.0f - 2.0f * x);
}

// Linear slew 0..1 used as an on/off gate. Passing it through smoothstep()
// gives click-free engage/release of an effect.
class Ramp
{
public:
    void prepare (double sampleRate, double ms) noexcept
    {
        inc = 1.0f / std::max (1.0f, (float) (sampleRate * ms * 0.001));
    }
    void reset (float v = 0.0f) noexcept { value = target = v; }
    void setTarget (float t) noexcept    { target = t; }
    float next() noexcept
    {
        if (value < target)      value = std::min (target, value + inc);
        else if (value > target) value = std::max (target, value - inc);
        return value;
    }

    float value = 0.0f, target = 0.0f, inc = 0.01f;
};
} // namespace gz
