#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

namespace gz
{
namespace col
{
    inline const juce::Colour bg        { 0xff0a0a10 };
    inline const juce::Colour panel     { 0xff13131b };
    inline const juce::Colour panelEdge { 0xff23232f };
    inline const juce::Colour track     { 0xff2a2a38 };
    inline const juce::Colour accent    { 0xff8b5cf6 };   // violet
    inline const juce::Colour accent2   { 0xff6d8cd6 };   // muted blue
    inline const juce::Colour text      { 0xffe8eaf2 };   // cold white
    inline const juce::Colour dim       { 0xff6c7086 };
}

class GazyavaLookAndFeel : public juce::LookAndFeel_V4
{
public:
    GazyavaLookAndFeel()
    {
        using juce::ComboBox; using juce::PopupMenu; using juce::Slider; using juce::Label;
        setColour (ComboBox::backgroundColourId, col::panelEdge);
        setColour (ComboBox::outlineColourId, juce::Colours::transparentBlack);
        setColour (ComboBox::textColourId, col::text);
        setColour (ComboBox::arrowColourId, col::accent);
        setColour (PopupMenu::backgroundColourId, col::panel);
        setColour (PopupMenu::textColourId, col::text);
        setColour (PopupMenu::highlightedBackgroundColourId, col::accent);
        setColour (PopupMenu::highlightedTextColourId, juce::Colours::white);
        setColour (Slider::textBoxTextColourId, col::text);
        setColour (Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
        setColour (Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
        setColour (Label::textColourId, col::text);
    }

    void drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos,
                           float startAngle, float endAngle, juce::Slider&) override
    {
        auto b = juce::Rectangle<float> ((float) x, (float) y, (float) w, (float) h).reduced (6.0f);
        const float r = juce::jmin (b.getWidth(), b.getHeight()) * 0.5f;
        const auto c = b.getCentre();
        const float arcR = r - 2.0f;
        const float ang = startAngle + pos * (endAngle - startAngle);

        juce::Path trackPath;
        trackPath.addCentredArc (c.x, c.y, arcR, arcR, 0.0f, startAngle, endAngle, true);
        g.setColour (col::track);
        g.strokePath (trackPath, juce::PathStrokeType (3.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        juce::Path valuePath;
        valuePath.addCentredArc (c.x, c.y, arcR, arcR, 0.0f, startAngle, ang, true);
        g.setColour (col::accent.withAlpha (0.25f));
        g.strokePath (valuePath, juce::PathStrokeType (7.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded)); // glow
        g.setColour (col::accent);
        g.strokePath (valuePath, juce::PathStrokeType (3.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        const float bodyR = r - 9.0f;
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xff2c2c3a), c.x, c.y - bodyR,
                                                 juce::Colour (0xff15151d), c.x, c.y + bodyR, false));
        g.fillEllipse (c.x - bodyR, c.y - bodyR, bodyR * 2.0f, bodyR * 2.0f);
        g.setColour (col::panelEdge);
        g.drawEllipse (c.x - bodyR, c.y - bodyR, bodyR * 2.0f, bodyR * 2.0f, 1.0f);

        juce::Path pointer;
        pointer.addRoundedRectangle (-1.5f, -bodyR + 3.0f, 3.0f, bodyR * 0.42f, 1.5f);
        g.setColour (col::text);
        g.fillPath (pointer, juce::AffineTransform::rotation (ang).translated (c.x, c.y));
    }

    juce::Font getComboBoxFont (juce::ComboBox&) override
    {
        return juce::Font (juce::FontOptions (13.0f, juce::Font::bold));
    }
};
} // namespace gz
