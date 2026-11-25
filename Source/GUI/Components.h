#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include "GazyavaLookAndFeel.h"

namespace gz
{
/** Pill-shaped button with an animated on/off transition.
    Can also be "momentary" (bound to a bool parameter: 1 while pressed) - used for CAPTURE. */
class PillButton : public juce::Button, private juce::Timer
{
public:
    explicit PillButton (const juce::String& text, bool big = false)
        : juce::Button (text), isBig (big)
    {
        setClickingTogglesState (true);
    }

    void setMomentaryParameter (juce::RangedAudioParameter* param)
    {
        momentaryParam = param;
        setClickingTogglesState (false);
    }

    void paintButton (juce::Graphics& g, bool over, bool) override
    {
        if (std::abs (anim - targetValue()) > 0.001f) startTimerHz (60);

        auto b = getLocalBounds().toFloat().reduced (3.0f);
        const float radius = b.getHeight() * 0.5f;

        if (anim > 0.02f)
        {
            g.setColour (col::accent.withAlpha (0.28f * anim));
            g.fillRoundedRectangle (b.expanded (2.5f), radius + 2.5f);      // glow
        }
        auto base = col::panelEdge.interpolatedWith (col::accent, anim);
        if (over) base = base.brighter (0.1f);
        g.setColour (base);
        g.fillRoundedRectangle (b, radius);

        g.setColour (col::dim.interpolatedWith (juce::Colours::white, anim));
        g.setFont (juce::Font (juce::FontOptions (isBig ? 17.0f : 12.5f, juce::Font::bold)));
        g.drawText (getButtonText(), getLocalBounds(), juce::Justification::centred);
    }

    void buttonStateChanged() override
    {
        if (momentaryParam == nullptr) return;
        const bool d = isDown();
        if (d && ! lastDown)
        {
            lastDown = true; releasePending = false;
            momentaryParam->beginChangeGesture();
            momentaryParam->setValueNotifyingHost (1.0f);
        }
        else if (! d && lastDown)
        {
            // Keep the parameter at 1 for at least 80 ms so the audio thread cannot miss a short click.
            lastDown = false; releasePending = true;
            releaseAt = juce::Time::getMillisecondCounter() + 80;
            startTimerHz (60);
        }
    }

private:
    float targetValue() const { return momentaryParam != nullptr ? (isDown() || releasePending ? 1.0f : 0.0f)
                                                                 : (getToggleState() ? 1.0f : 0.0f); }
    void timerCallback() override
    {
        if (releasePending && juce::Time::getMillisecondCounter() >= releaseAt)
        {
            releasePending = false;
            momentaryParam->setValueNotifyingHost (0.0f);
            momentaryParam->endChangeGesture();
        }
        const float t = targetValue();
        anim += (t - anim) * 0.28f;
        if (std::abs (t - anim) < 0.01f) { anim = t; if (! releasePending) stopTimer(); }
        repaint();
    }

    float anim = 0.0f;
    bool isBig = false, lastDown = false, releasePending = false;
    juce::uint32 releaseAt = 0;
    juce::RangedAudioParameter* momentaryParam = nullptr;
};

/** Rotary knob + caption, bound to a parameter. */
class Knob : public juce::Component
{
public:
    Knob (juce::AudioProcessorValueTreeState& apvts, const juce::String& paramId, const juce::String& title)
    {
        slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 72, 16);
        slider.setRotaryParameters (juce::MathConstants<float>::pi * 1.25f,
                                    juce::MathConstants<float>::pi * 2.75f, true);
        addAndMakeVisible (slider);

        label.setText (title.toUpperCase(), juce::dontSendNotification);
        label.setJustificationType (juce::Justification::centred);
        label.setFont (juce::Font (juce::FontOptions (11.0f, juce::Font::bold)));
        label.setColour (juce::Label::textColourId, col::dim);
        addAndMakeVisible (label);

        attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (apvts, paramId, slider);
        if (auto* prm = apvts.getParameter (paramId))
            slider.setDoubleClickReturnValue (true, prm->convertFrom0to1 (prm->getDefaultValue()));
    }

    void resized() override
    {
        auto b = getLocalBounds();
        label.setBounds (b.removeFromBottom (16));
        slider.setBounds (b);
    }

private:
    juce::Slider slider;
    juce::Label label;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;   // must be last
};

/** Horizontal peak meter fed from an atomic written by the audio thread. */
class LevelMeter : public juce::Component, private juce::Timer
{
public:
    LevelMeter (std::atomic<float>& src, const juce::String& name) : source (src), title (name)
    {
        startTimerHz (30);
    }

    void paint (juce::Graphics& g) override
    {
        auto b = getLocalBounds().toFloat();
        auto label = b.removeFromLeft (34.0f);
        g.setColour (col::dim);
        g.setFont (juce::Font (juce::FontOptions (11.0f, juce::Font::bold)));
        g.drawText (title, label.toNearestInt(), juce::Justification::centredLeft);

        auto bar = b.withSizeKeepingCentre (b.getWidth(), 8.0f);
        g.setColour (col::track);
        g.fillRoundedRectangle (bar, 4.0f);

        auto fill = bar.withWidth (bar.getWidth() * level);
        g.setGradientFill (juce::ColourGradient (col::accent2, bar.getX(), 0.0f,
                                                 col::accent, bar.getRight(), 0.0f, false));
        g.fillRoundedRectangle (fill, 4.0f);
    }

private:
    void timerCallback() override
    {
        const float peak = source.exchange (0.0f);
        const float db = juce::Decibels::gainToDecibels (peak, -60.0f);
        const float v = juce::jmap (db, -60.0f, 0.0f, 0.0f, 1.0f);
        level = juce::jmax (v, level - 0.04f);       // instant attack, smooth release
        repaint();
    }

    std::atomic<float>& source;
    juce::String title;
    float level = 0.0f;
};
} // namespace gz
