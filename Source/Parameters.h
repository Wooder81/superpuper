#pragma once
#include <juce_audio_processors/juce_audio_processors.h>

namespace gz
{
namespace pid
{
    inline constexpr const char* bypass = "bypass";
    inline constexpr const char* output = "output";

    inline constexpr const char* stOn = "stOn";
    inline constexpr const char* stCapture = "stCapture";
    inline constexpr const char* stHold = "stHold";
    inline constexpr const char* stSync = "stSync";
    inline constexpr const char* stDiv = "stDiv";
    inline constexpr const char* stLenMs = "stLenMs";
    inline constexpr const char* stSpeed = "stSpeed";
    inline constexpr const char* stPitch = "stPitch";
    inline constexpr const char* stMix = "stMix";
    inline constexpr const char* stFade = "stFade";
    inline constexpr const char* stDuration = "stDuration";

    inline constexpr const char* rvOn = "rvOn";
    inline constexpr const char* rvMode = "rvMode";
    inline constexpr const char* rvSync = "rvSync";
    inline constexpr const char* rvDiv = "rvDiv";
    inline constexpr const char* rvLenMs = "rvLenMs";
    inline constexpr const char* rvStart = "rvStart";
    inline constexpr const char* rvEnd = "rvEnd";
    inline constexpr const char* rvMix = "rvMix";
    inline constexpr const char* rvVol = "rvVol";
    inline constexpr const char* rvFade = "rvFade";
    inline constexpr const char* rvSpeed = "rvSpeed";
    inline constexpr const char* rvTransition = "rvTransition";
    inline constexpr const char* rvAutoReturn = "rvAutoReturn";
}

juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
} // namespace gz
