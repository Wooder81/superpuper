#include "PluginEditor.h"

namespace
{
void fillCombo (juce::AudioProcessorValueTreeState& apvts, juce::ComboBox& box, const char* id)
{
    if (auto* c = dynamic_cast<juce::AudioParameterChoice*> (apvts.getParameter (id)))
        box.addItemList (c->choices, 1);
}

void placeGrid (juce::Rectangle<int> area, std::vector<juce::Component*> items, int cols)
{
    const int rows = ((int) items.size() + cols - 1) / cols;
    const int cw = area.getWidth() / cols, ch = area.getHeight() / rows;
    for (int i = 0; i < (int) items.size(); ++i)
        items[(size_t) i]->setBounds (area.getX() + (i % cols) * cw, area.getY() + (i / cols) * ch, cw, ch);
}
}

GazyavaEditor::GazyavaEditor (GazyavaAudioProcessor& p)
    : AudioProcessorEditor (&p), proc (p),
      stSpeed (p.apvts, gz::pid::stSpeed, "Speed"), stLen (p.apvts, gz::pid::stLenMs, "Length"),
      stPitch (p.apvts, gz::pid::stPitch, "Pitch"), stMix (p.apvts, gz::pid::stMix, "Mix"),
      stFade (p.apvts, gz::pid::stFade, "Fade"),    stDur (p.apvts, gz::pid::stDuration, "Duration"),
      rvLen (p.apvts, gz::pid::rvLenMs, "Length"),  rvSpeed (p.apvts, gz::pid::rvSpeed, "Speed"),
      rvStart (p.apvts, gz::pid::rvStart, "Start"), rvEnd (p.apvts, gz::pid::rvEnd, "End"),
      rvMix (p.apvts, gz::pid::rvMix, "Mix"),       rvVol (p.apvts, gz::pid::rvVol, "Volume"),
      rvFade (p.apvts, gz::pid::rvFade, "Fade"),    rvTrans (p.apvts, gz::pid::rvTransition, "Transition"),
      outKnob (p.apvts, gz::pid::output, "Output"),
      inMeter (p.inPeak, "IN"), outMeter (p.outPeak, "OUT")
{
    setLookAndFeel (&laf);

    fillCombo (p.apvts, stDiv, gz::pid::stDiv);
    fillCombo (p.apvts, rvMode, gz::pid::rvMode);
    fillCombo (p.apvts, rvDiv, gz::pid::rvDiv);

    for (juce::Component* c : std::initializer_list<juce::Component*> {
             &bypass, &stOn, &stCapture, &stHold, &stSync, &rvOn, &rvSync, &rvAuto,
             &stDiv, &rvMode, &rvDiv,
             &stSpeed, &stLen, &stPitch, &stMix, &stFade, &stDur,
             &rvLen, &rvSpeed, &rvStart, &rvEnd, &rvMix, &rvVol, &rvFade, &rvTrans,
             &outKnob, &inMeter, &outMeter })
        addAndMakeVisible (*c);

    stCapture.setMomentaryParameter (p.apvts.getParameter (gz::pid::stCapture));

    aBypass = std::make_unique<BtnAtt> (p.apvts, gz::pid::bypass, bypass);
    aStOn   = std::make_unique<BtnAtt> (p.apvts, gz::pid::stOn, stOn);
    aStHold = std::make_unique<BtnAtt> (p.apvts, gz::pid::stHold, stHold);
    aStSync = std::make_unique<BtnAtt> (p.apvts, gz::pid::stSync, stSync);
    aRvOn   = std::make_unique<BtnAtt> (p.apvts, gz::pid::rvOn, rvOn);
    aRvSync = std::make_unique<BtnAtt> (p.apvts, gz::pid::rvSync, rvSync);
    aRvAuto = std::make_unique<BtnAtt> (p.apvts, gz::pid::rvAutoReturn, rvAuto);
    aStDiv  = std::make_unique<ComboAtt> (p.apvts, gz::pid::stDiv, stDiv);
    aRvMode = std::make_unique<ComboAtt> (p.apvts, gz::pid::rvMode, rvMode);
    aRvDiv  = std::make_unique<ComboAtt> (p.apvts, gz::pid::rvDiv, rvDiv);

    setSize (940, 600);
    startTimerHz (30);
}

GazyavaEditor::~GazyavaEditor() { setLookAndFeel (nullptr); }

void GazyavaEditor::paint (juce::Graphics& g)
{
    namespace c = gz::col;
    g.setGradientFill (juce::ColourGradient (c::bg.brighter (0.03f), 0.0f, 0.0f, c::bg, 0.0f, (float) getHeight(), false));
    g.fillAll();

    // logo
    g.setColour (c::text);
    g.setFont (juce::Font (juce::FontOptions (32.0f, juce::Font::bold)));
    g.drawText ("GAZYAVA", 24, 14, 220, 40, juce::Justification::centredLeft);
    g.setColour (c::dim);
    g.setFont (juce::Font (juce::FontOptions (11.0f, juce::Font::bold)));
    g.drawText ("VOCAL STUTTER / REVERSE", 26, 46, 240, 14, juce::Justification::centredLeft);

    // activity indicator
    const bool any = stActive || rvActive;
    const float glow = any ? 0.55f + 0.45f * std::sin (pulse) : 0.0f;
    const auto dot = juce::Rectangle<float> (270.0f, 27.0f, 12.0f, 12.0f);
    if (any) { g.setColour (c::accent.withAlpha (0.3f * glow)); g.fillEllipse (dot.expanded (5.0f)); }
    g.setColour (any ? c::accent : c::track);
    g.fillEllipse (dot);
    g.setColour (any ? c::text : c::dim);
    g.setFont (juce::Font (juce::FontOptions (12.0f, juce::Font::bold)));
    g.drawText (any ? "ACTIVE" : "IDLE", 290, 24, 80, 18, juce::Justification::centredLeft);

    // module cards
    for (auto* card : { &stCard, &rvCard })
    {
        auto r = card->toFloat();
        g.setColour (c::panel);
        g.fillRoundedRectangle (r, 16.0f);
        g.setColour (c::panelEdge);
        g.drawRoundedRectangle (r.reduced (0.5f), 16.0f, 1.0f);
    }
    auto drawDot = [&] (juce::Rectangle<int> card, bool active)
    {
        auto d = juce::Rectangle<float> ((float) card.getRight() - 40.0f, (float) card.getY() + 30.0f, 10.0f, 10.0f);
        if (active) { g.setColour (c::accent.withAlpha (0.3f * (0.55f + 0.45f * std::sin (pulse)))); g.fillEllipse (d.expanded (4.0f)); }
        g.setColour (active ? c::accent : c::track);
        g.fillEllipse (d);
    };
    drawDot (stCard, stActive);
    drawDot (rvCard, rvActive);

    // BPM / sync indication
    g.setColour (c::dim);
    g.setFont (juce::Font (juce::FontOptions (12.0f, juce::Font::bold)));
    const bool host = proc.hostTempo.load();
    g.drawText ("BPM " + juce::String (proc.bpmDisplay.load(), 1) + (host ? "   |   HOST SYNC" : "   |   NO HOST TEMPO (120)"),
                bpmRect, juce::Justification::centredLeft);
}

void GazyavaEditor::resized()
{
    auto area = getLocalBounds().reduced (20);
    auto top = area.removeFromTop (56);
    topRect = top;
    bypass.setBounds (top.removeFromRight (110).withSizeKeepingCentre (110, 34));

    auto bottom = area.removeFromBottom (80);
    area.removeFromBottom (10);

    stCard = area.removeFromLeft ((area.getWidth() - 16) / 2);
    area.removeFromLeft (16);
    rvCard = area;
    layoutStutter (stCard.reduced (16));
    layoutReverse (rvCard.reduced (16));

    auto meters = bottom.removeFromLeft (300);
    inMeter.setBounds (meters.removeFromTop (24));
    meters.removeFromTop (6);
    outMeter.setBounds (meters.removeFromTop (24));
    outKnob.setBounds (bottom.removeFromRight (100));
    bpmRect = bottom.withTrimmedLeft (24);
}

void GazyavaEditor::layoutStutter (juce::Rectangle<int> r)
{
    auto header = r.removeFromTop (52);
    stOn.setBounds (header.removeFromLeft (190).withSizeKeepingCentre (190, 44));
    r.removeFromTop (6);

    auto row = r.removeFromTop (34);
    stCapture.setBounds (row.removeFromLeft (110)); row.removeFromLeft (8);
    stHold.setBounds    (row.removeFromLeft (80));  row.removeFromLeft (8);
    stSync.setBounds    (row.removeFromLeft (80));  row.removeFromLeft (8);
    stDiv.setBounds     (row.removeFromLeft (84).reduced (0, 3));

    r.removeFromTop (10);
    placeGrid (r, { &stSpeed, &stLen, &stPitch, &stMix, &stFade, &stDur }, 3);
}

void GazyavaEditor::layoutReverse (juce::Rectangle<int> r)
{
    auto header = r.removeFromTop (52);
    rvOn.setBounds (header.removeFromLeft (190).withSizeKeepingCentre (190, 44));
    r.removeFromTop (6);

    auto row = r.removeFromTop (34);
    rvMode.setBounds (row.removeFromLeft (112).reduced (0, 3)); row.removeFromLeft (8);
    rvSync.setBounds (row.removeFromLeft (70));  row.removeFromLeft (8);
    rvDiv.setBounds  (row.removeFromLeft (76).reduced (0, 3));  row.removeFromLeft (8);
    rvAuto.setBounds (row.removeFromLeft (110));

    r.removeFromTop (10);
    placeGrid (r, { &rvLen, &rvSpeed, &rvStart, &rvEnd, &rvMix, &rvVol, &rvFade, &rvTrans }, 4);
}

void GazyavaEditor::timerCallback()
{
    const bool st = proc.stutterActive.load(), rv = proc.reverseActive.load();
    pulse += 0.25f;

    // grey out whichever length control is not in use (sync vs manual)
    const bool stSyncOn = proc.apvts.getRawParameterValue (gz::pid::stSync)->load() > 0.5f;
    const bool rvSyncOn = proc.apvts.getRawParameterValue (gz::pid::rvSync)->load() > 0.5f;
    stDiv.setAlpha (stSyncOn ? 1.0f : 0.35f);  stLen.setAlpha (stSyncOn ? 0.35f : 1.0f);
    rvDiv.setAlpha (rvSyncOn ? 1.0f : 0.35f);  rvLen.setAlpha (rvSyncOn ? 0.35f : 1.0f);

    stActive = st; rvActive = rv;
    repaint (topRect);
    repaint (bpmRect);
    repaint (stCard.withHeight (60));
    repaint (rvCard.withHeight (60));
}
