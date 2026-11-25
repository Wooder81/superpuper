#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
constexpr double kStFractions[5] = { 0.5, 0.25, 0.125, 0.0625, 0.03125 };       // of a bar
constexpr double kRvFractions[6] = { 0.0625, 0.125, 0.25, 0.5, 1.0, 2.0 };      // of a bar
}

GazyavaAudioProcessor::GazyavaAudioProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "GAZYAVA", gz::createParameterLayout())
{
    auto P = [this] (const char* id) { return apvts.getRawParameterValue (id); };
    namespace i = gz::pid;
    p = { P (i::bypass), P (i::output),
          P (i::stOn), P (i::stCapture), P (i::stHold), P (i::stSync), P (i::stDiv), P (i::stLenMs),
          P (i::stSpeed), P (i::stPitch), P (i::stMix), P (i::stFade), P (i::stDuration),
          P (i::rvOn), P (i::rvMode), P (i::rvSync), P (i::rvDiv), P (i::rvLenMs), P (i::rvStart),
          P (i::rvEnd), P (i::rvMix), P (i::rvVol), P (i::rvFade), P (i::rvSpeed),
          P (i::rvTransition), P (i::rvAutoReturn) };
}

bool GazyavaAudioProcessor::isBusesLayoutSupported (const BusesLayout& l) const
{
    const auto out = l.getMainOutputChannelSet();
    return (out == juce::AudioChannelSet::mono() || out == juce::AudioChannelSet::stereo())
           && out == l.getMainInputChannelSet();
}

void GazyavaAudioProcessor::prepareToPlay (double sr, int)
{
    // All memory is allocated here, never in processBlock.
    history.prepare (sr, 10.0);
    stutter.prepare (sr);
    reverse.prepare (sr);
    bypassRamp.prepare (sr, 15.0);
    bypassRamp.reset (p.bypass->load() > 0.5f ? 1.0f : 0.0f);
    outGain.reset (sr, 0.03);
    outGain.setCurrentAndTargetValue (gz::dbToGain (p.output->load()));
    setLatencySamples (0);      // effects are live: no look-ahead, zero latency
}

void GazyavaAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const int numCh = juce::jmin (buffer.getNumChannels(), 2);
    const int total = buffer.getNumSamples();
    if (numCh == 0 || total == 0) return;
    for (int ch = numCh; ch < buffer.getNumChannels(); ++ch) buffer.clear (ch, 0, total);

    // ---- host tempo / time signature (falls back to 120 BPM, 4/4) ----
    double bpm = 120.0, barQuarters = 4.0;
    bool fromHost = false;
    if (auto* ph = getPlayHead())
        if (auto pos = ph->getPosition())
        {
            if (auto b = pos->getBpm()) if (*b > 20.0 && *b < 400.0) { bpm = *b; fromHost = true; }
            if (auto ts = pos->getTimeSignature())
                if (ts->denominator > 0) barQuarters = ts->numerator * 4.0 / ts->denominator;
        }
    bpmDisplay.store (bpm);
    hostTempo.store (fromHost);

    const double sr = getSampleRate();
    const double samplesPerBar = sr * 60.0 / bpm * barQuarters;
    auto idx = [] (const std::atomic<float>* a, int maxI) { return juce::jlimit (0, maxI, (int) (a->load() + 0.5f)); };

    // ---- gather parameters once per block ----
    gz::StutterProcessor::Params sp;
    sp.on      = p.stOn->load() > 0.5f;
    sp.capture = p.stCapture->load() > 0.5f;
    sp.hold    = p.stHold->load() > 0.5f;
    sp.lengthSamples = (p.stSync->load() > 0.5f)
                         ? (float) (kStFractions[idx (p.stDiv, 4)] * samplesPerBar)
                         : p.stLenMs->load() * 0.001f * (float) sr;
    sp.speed = p.stSpeed->load();
    sp.pitchSemitones = p.stPitch->load();
    sp.mix = p.stMix->load() * 0.01f;
    sp.fadeMs = p.stFade->load();
    sp.durationSamples = (double) p.stDuration->load() * samplesPerBar;

    gz::ReverseProcessor::Params rp;
    rp.on   = p.rvOn->load() > 0.5f;
    rp.mode = idx (p.rvMode, 1);
    rp.windowSamples = (p.rvSync->load() > 0.5f)
                         ? (float) (kRvFractions[idx (p.rvDiv, 5)] * samplesPerBar)
                         : p.rvLenMs->load() * 0.001f * (float) sr;
    rp.startPct = p.rvStart->load();
    rp.endPct   = p.rvEnd->load();
    rp.mix      = p.rvMix->load() * 0.01f;
    rp.volume   = gz::dbToGain (p.rvVol->load());
    rp.fadeMs   = p.rvFade->load();
    rp.speed    = p.rvSpeed->load();
    rp.transition = p.rvTransition->load() * 0.01f;
    rp.autoReturn = p.rvAutoReturn->load() > 0.5f;

    bypassRamp.setTarget (p.bypass->load() > 0.5f ? 1.0f : 0.0f);
    outGain.setTargetValue (gz::dbToGain (p.output->load()));

    float* chL = buffer.getWritePointer (0);
    float* chR = numCh > 1 ? buffer.getWritePointer (1) : nullptr;
    float inPk = 0.0f, outPk = 0.0f;

    // ---- process in fixed sub-blocks so any host buffer size is safe ----
    for (int off = 0; off < total; off += kChunk)
    {
        const int n = juce::jmin (kChunk, total - off);
        float* l = chL + off;
        float* r = chR != nullptr ? chR + off : monoScratch.data();
        if (chR == nullptr) std::copy (l, l + n, r);          // mono: process as dual-mono

        for (int i = 0; i < n; ++i) inPk = juce::jmax (inPk, std::abs (l[i]), std::abs (r[i]));
        std::copy (l, l + n, dryL.data());
        std::copy (r, r + n, dryR.data());

        history.write (l, r, n);                               // always recording the vocal
        stutter.process (l, r, n, sp, history);
        reverse.process (l, r, n, rp, history);

        for (int i = 0; i < n; ++i)
        {
            const float g = outGain.getNextValue();
            const float b = gz::smoothstep (bypassRamp.next());   // click-free bypass crossfade
            const float pl = gz::softClip (l[i] * g);             // clip protection on the wet path
            const float pr = gz::softClip (r[i] * g);
            l[i] = pl + (dryL[(size_t) i] * g - pl) * b;
            r[i] = pr + (dryR[(size_t) i] * g - pr) * b;
            outPk = juce::jmax (outPk, std::abs (l[i]), std::abs (r[i]));
        }
    }

    inPeak.store  (juce::jmax (inPeak.load(),  inPk));
    outPeak.store (juce::jmax (outPeak.load(), outPk));
    stutterActive.store (stutter.isActive());
    reverseActive.store (reverse.isActive());
}

juce::AudioProcessorEditor* GazyavaAudioProcessor::createEditor() { return new GazyavaEditor (*this); }

void GazyavaAudioProcessor::getStateInformation (juce::MemoryBlock& dest)
{
    auto state = apvts.copyState();
    if (auto xml = state.createXml()) copyXmlToBinary (*xml, dest);
}

void GazyavaAudioProcessor::setStateInformation (const void* data, int size)
{
    if (auto xml = getXmlFromBinary (data, size))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new GazyavaAudioProcessor(); }
