#include "PluginProcessor.h"
#include "PluginEditor.h"

EX2DetectProcessor::EX2DetectProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true))
{
}

void EX2DetectProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    analyzer.prepare (sampleRate);
    monoScratch.assign ((size_t) juce::jmax (16, samplesPerBlock), 0.0f);
}

bool EX2DetectProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto in  = layouts.getMainInputChannelSet();
    const auto out = layouts.getMainOutputChannelSet();
    if (in != out) return false;
    return in == juce::AudioChannelSet::mono() || in == juce::AudioChannelSet::stereo();
}

void EX2DetectProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    const int n  = buffer.getNumSamples();
    const int ch = buffer.getNumChannels();
    if (n == 0) return;

    if ((int) monoScratch.size() < n)
        monoScratch.assign ((size_t) n, 0.0f);

    const float inv = 1.0f / (float) juce::jmax (1, ch);
    for (int i = 0; i < n; ++i)
    {
        float s = 0.0f;
        for (int c = 0; c < ch; ++c)
            s += buffer.getReadPointer (c)[i];
        monoScratch[(size_t) i] = s * inv;
    }
    analyzer.push (monoScratch.data(), n);

    // audio passes through completely untouched
}

juce::AudioProcessorEditor* EX2DetectProcessor::createEditor()
{
    return new EX2DetectEditor (*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new EX2DetectProcessor();
}
