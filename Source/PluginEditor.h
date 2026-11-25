#pragma once
#include "PluginProcessor.h"
#include "GUI/GazyavaLookAndFeel.h"
#include "GUI/Components.h"

class GazyavaEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit GazyavaEditor (GazyavaAudioProcessor&);
    ~GazyavaEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    using BtnAtt   = juce::AudioProcessorValueTreeState::ButtonAttachment;
    using ComboAtt = juce::AudioProcessorValueTreeState::ComboBoxAttachment;

    void timerCallback() override;
    void layoutStutter (juce::Rectangle<int> r);
    void layoutReverse (juce::Rectangle<int> r);

    GazyavaAudioProcessor& proc;
    gz::GazyavaLookAndFeel laf;

    gz::PillButton bypass { "BYPASS" };
    gz::PillButton stOn { "STUTTER", true }, stCapture { "CAPTURE" }, stHold { "HOLD" }, stSync { "SYNC" };
    gz::PillButton rvOn { "REVERSE", true }, rvSync { "SYNC" }, rvAuto { "AUTO RETURN" };
    juce::ComboBox stDiv, rvMode, rvDiv;

    gz::Knob stSpeed, stLen, stPitch, stMix, stFade, stDur;
    gz::Knob rvLen, rvSpeed, rvStart, rvEnd, rvMix, rvVol, rvFade, rvTrans;
    gz::Knob outKnob;
    gz::LevelMeter inMeter, outMeter;

    juce::Rectangle<int> stCard, rvCard, bpmRect, topRect;
    bool stActive = false, rvActive = false;
    float pulse = 0.0f;

    std::unique_ptr<BtnAtt> aBypass, aStOn, aStHold, aStSync, aRvOn, aRvSync, aRvAuto;
    std::unique_ptr<ComboAtt> aStDiv, aRvMode, aRvDiv;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GazyavaEditor)
};
