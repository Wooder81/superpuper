#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <array>
#include <atomic>
#include "Parameters.h"
#include "DSP/AudioBufferManager.h"
#include "DSP/StutterProcessor.h"
#include "DSP/ReverseProcessor.h"
#include "DSP/DspUtils.h"

class GazyavaAudioProcessor : public juce::AudioProcessor
{
public:
    GazyavaAudioProcessor();
    ~GazyavaAudioProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout&) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "GAZYAVA"; }
    bool acceptsMidi() const override  { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    juce::AudioProcessorValueTreeState apvts;

    // Read by the editor (lock-free)
    std::atomic<float>  inPeak { 0.0f }, outPeak { 0.0f };
    std::atomic<bool>   stutterActive { false }, reverseActive { false }, hostTempo { false };
    std::atomic<double> bpmDisplay { 120.0 };

private:
    static constexpr int kChunk = 1024;     // internal sub-block size: works for ANY host buffer size

    struct Ptrs
    {
        std::atomic<float> *bypass, *output,
            *stOn, *stCapture, *stHold, *stSync, *stDiv, *stLenMs, *stSpeed, *stPitch, *stMix, *stFade, *stDuration,
            *rvOn, *rvMode, *rvSync, *rvDiv, *rvLenMs, *rvStart, *rvEnd, *rvMix, *rvVol, *rvFade, *rvSpeed,
            *rvTransition, *rvAutoReturn;
    } p {};

    gz::AudioBufferManager history;
    gz::StutterProcessor stutter;
    gz::ReverseProcessor reverse;
    gz::Ramp bypassRamp;
    juce::SmoothedValue<float> outGain;

    std::array<float, kChunk> dryL {}, dryR {}, monoScratch {};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GazyavaAudioProcessor)
};
