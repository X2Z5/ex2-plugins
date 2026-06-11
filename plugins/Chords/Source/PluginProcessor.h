#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include "ChordEngine.h"

class EX2ChordsProcessor : public juce::AudioProcessor,
                           private juce::AudioProcessorValueTreeState::Listener
{
public:
    EX2ChordsProcessor();
    ~EX2ChordsProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override                 { return true; }
    const juce::String getName() const override     { return JucePlugin_Name; }
    bool acceptsMidi() const override               { return false; }
    bool producesMidi() const override              { return true; }
    bool isMidiEffect() const override              { return false; }
    double getTailLengthSeconds() const override    { return 0.2; }
    int getNumPrograms() override                   { return 1; }
    int getCurrentProgram() override                { return 0; }
    void setCurrentProgram (int) override           {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}
    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    juce::AudioProcessorValueTreeState apvts;

    struct DisplayChord { juce::String name; double lengthBars; };
    std::vector<DisplayChord> getDisplayChords();

    void reroll();                       // dice: random progression/rhythm/feel
    juce::File writeMidiToTemp();        // for drag & drop
    bool writeMidiTo (const juce::File&);
    double currentBpm() const            { return lastBpm.load(); }

private:
    void parameterChanged (const juce::String&, float) override { dirty = true; }
    void rebuildIfNeeded();
    void scheduleRange (double fromBeat, double toBeat, juce::MidiBuffer&, int numSamples, double samplesPerBeat);
    void sendAllNotesOff (juce::MidiBuffer&, int samplePos);
    static juce::AudioProcessorValueTreeState::ParameterLayout makeLayout();

    juce::Synthesiser synth;
    std::vector<ex2::ChordEvent> events;
    std::vector<DisplayChord> displayChords;
    double totalBeats = 16.0;
    juce::CriticalSection eventLock;
    std::atomic<bool> dirty { true };
    std::atomic<double> lastBpm { 120.0 };
    juce::uint32 seed = 0x20260611;

    struct PendingOff { int note; int samplesLeft; };
    std::vector<PendingOff> pendingOffs;
    double playCursor = -1.0;
    bool wasActive = false, previewWas = false;
    double sampleRateHz = 44100.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EX2ChordsProcessor)
};
