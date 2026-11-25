#include "Parameters.h"

namespace gz
{
using APF = juce::AudioParameterFloat;
using APB = juce::AudioParameterBool;
using APC = juce::AudioParameterChoice;

static std::unique_ptr<APF> makeFloat (const char* id, const juce::String& name,
                                       juce::NormalisableRange<float> range, float def,
                                       const juce::String& unit, int decimals)
{
    auto attr = juce::AudioParameterFloatAttributes()
        .withStringFromValueFunction ([unit, decimals] (float v, int)
                                      { return juce::String (v, decimals) + (unit.isEmpty() ? juce::String() : " " + unit); })
        .withValueFromStringFunction ([] (const juce::String& t) { return t.getFloatValue(); });
    return std::make_unique<APF> (juce::ParameterID { id, 1 }, name, range, def, attr);
}

static std::unique_ptr<APB> makeBool (const char* id, const juce::String& name, bool def)
{
    return std::make_unique<APB> (juce::ParameterID { id, 1 }, name, def);
}

static std::unique_ptr<APC> makeChoice (const char* id, const juce::String& name,
                                        const juce::StringArray& items, int def)
{
    return std::make_unique<APC> (juce::ParameterID { id, 1 }, name, items, def);
}

juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
{
    using R = juce::NormalisableRange<float>;
    juce::AudioProcessorValueTreeState::ParameterLayout l;

    l.add (makeBool  (pid::bypass, "Bypass", false));
    l.add (makeFloat (pid::output, "Output", R (-24.0f, 12.0f, 0.1f), 0.0f, "dB", 1));

    // ---------------- STUTTER ----------------
    l.add (makeBool   (pid::stOn,      "Stutter On",      false));
    l.add (makeBool   (pid::stCapture, "Stutter Capture", false));   // momentary
    l.add (makeBool   (pid::stHold,    "Stutter Hold",    false));
    l.add (makeBool   (pid::stSync,    "Stutter Sync",    true));
    l.add (makeChoice (pid::stDiv,     "Stutter Division", { "1/2", "1/4", "1/8", "1/16", "1/32" }, 2));
    { R r (5.0f, 2000.0f, 1.0f); r.setSkewForCentre (200.0f);
      l.add (makeFloat (pid::stLenMs, "Stutter Length", r, 250.0f, "ms", 0)); }
    { R r (0.25f, 4.0f, 0.01f); r.setSkewForCentre (1.0f);
      l.add (makeFloat (pid::stSpeed, "Stutter Speed", r, 1.0f, "x", 2)); }
    l.add (makeFloat (pid::stPitch, "Stutter Pitch", R (-12.0f, 12.0f, 0.01f), 0.0f, "st", 1));
    l.add (makeFloat (pid::stMix,   "Stutter Mix",   R (0.0f, 100.0f, 0.1f), 100.0f, "%", 0));
    { R r (1.0f, 100.0f, 0.1f); r.setSkewForCentre (12.0f);
      l.add (makeFloat (pid::stFade, "Stutter Fade", r, 8.0f, "ms", 1)); }
    { R r (0.0625f, 8.0f, 0.0001f); r.setSkewForCentre (1.0f);
      l.add (makeFloat (pid::stDuration, "Stutter Duration", r, 1.0f, "bar", 2)); }

    // ---------------- REVERSE ----------------
    l.add (makeBool   (pid::rvOn,   "Reverse On",   false));
    l.add (makeChoice (pid::rvMode, "Reverse Mode", { "Reverse", "Transition" }, 1));
    l.add (makeBool   (pid::rvSync, "Reverse Sync", true));
    l.add (makeChoice (pid::rvDiv,  "Reverse Division", { "1/16", "1/8", "1/4", "1/2", "1 bar", "2 bars" }, 2));
    { R r (50.0f, 4000.0f, 1.0f); r.setSkewForCentre (600.0f);
      l.add (makeFloat (pid::rvLenMs, "Reverse Length", r, 600.0f, "ms", 0)); }
    l.add (makeFloat (pid::rvStart, "Reverse Start", R (0.0f, 100.0f, 0.1f), 0.0f,   "%", 0));
    l.add (makeFloat (pid::rvEnd,   "Reverse End",   R (0.0f, 100.0f, 0.1f), 100.0f, "%", 0));
    l.add (makeFloat (pid::rvMix,   "Reverse Mix",   R (0.0f, 100.0f, 0.1f), 100.0f, "%", 0));
    l.add (makeFloat (pid::rvVol,   "Reverse Volume", R (-24.0f, 6.0f, 0.1f), 0.0f, "dB", 1));
    { R r (1.0f, 500.0f, 0.1f); r.setSkewForCentre (40.0f);
      l.add (makeFloat (pid::rvFade, "Reverse Fade", r, 20.0f, "ms", 0)); }
    { R r (0.5f, 2.0f, 0.01f); r.setSkewForCentre (1.0f);
      l.add (makeFloat (pid::rvSpeed, "Reverse Speed", r, 1.0f, "x", 2)); }
    l.add (makeFloat (pid::rvTransition, "Reverse Transition", R (0.0f, 100.0f, 0.1f), 30.0f, "%", 0));
    l.add (makeBool  (pid::rvAutoReturn, "Reverse Auto Return", true));

    return l;
}
} // namespace gz
