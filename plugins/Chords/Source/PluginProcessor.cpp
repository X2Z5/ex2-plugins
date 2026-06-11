#include "PluginProcessor.h"
#include "PluginEditor.h"

juce::AudioProcessorValueTreeState::ParameterLayout EX2ChordsProcessor::makeLayout()
{
    using namespace juce;
    AudioProcessorValueTreeState::ParameterLayout layout;

    StringArray keys { "C","C#","D","D#","E","F","F#","G","G#","A","A#","B" };
    StringArray progs, rhys;
    for (const auto& p : ex2::progressions()) progs.add (p.name);
    for (const auto& r : ex2::rhythms())      rhys.add (r.name);

    layout.add (std::make_unique<AudioParameterChoice> (ParameterID { "key", 1 },      "Key", keys, 0));
    layout.add (std::make_unique<AudioParameterChoice> (ParameterID { "prog", 1 },     "Progression", progs, 0));
    layout.add (std::make_unique<AudioParameterChoice> (ParameterID { "rhythm", 1 },   "Bounce", rhys, 2));
    layout.add (std::make_unique<AudioParameterChoice> (ParameterID { "ext", 1 },      "Chord Flavor",
                    StringArray { "Simple Triads", "7th Chords", "Lush 9ths" }, 1));
    layout.add (std::make_unique<AudioParameterChoice> (ParameterID { "bass", 1 },     "Bass",
                    StringArray { "No Bass", "Root Bass", "Octave Bounce" }, 1));
    layout.add (std::make_unique<AudioParameterChoice> (ParameterID { "octave", 1 },   "Octave",
                    StringArray { "-1", "0", "+1" }, 1));
    layout.add (std::make_unique<AudioParameterFloat>  (ParameterID { "humanize", 1 }, "Humanize",
                    NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.35f));
    layout.add (std::make_unique<AudioParameterBool>   (ParameterID { "preview", 1 },  "Preview", false));
    layout.add (std::make_unique<AudioParameterBool>   (ParameterID { "hostmidi", 1 }, "MIDI Out On Play", true));
    return layout;
}

EX2ChordsProcessor::EX2ChordsProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "params", makeLayout())
{
    for (const char* id : { "key", "prog", "rhythm", "ext", "bass", "octave", "humanize" })
        apvts.addParameterListener (id, this);

    for (int i = 0; i < 12; ++i)
        synth.addVoice (new ex2::SawVoice());
    synth.addSound (new ex2::AnySound());
}

EX2ChordsProcessor::~EX2ChordsProcessor()
{
    for (const char* id : { "key", "prog", "rhythm", "ext", "bass", "octave", "humanize" })
        apvts.removeParameterListener (id, this);
}

void EX2ChordsProcessor::prepareToPlay (double sampleRate, int)
{
    sampleRateHz = sampleRate;
    synth.setCurrentPlaybackSampleRate (sampleRate);
    pendingOffs.clear();
    playCursor = -1.0;
    wasActive = false;
}

bool EX2ChordsProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet();
    return out == juce::AudioChannelSet::stereo() || out == juce::AudioChannelSet::mono();
}

void EX2ChordsProcessor::rebuildIfNeeded()
{
    if (! dirty.exchange (false))
        return;

    auto gi = [this] (const char* id) { return (int) std::lround (apvts.getRawParameterValue (id)->load()); };

    auto res = ex2::generate (gi ("key"), gi ("prog"), gi ("rhythm"), gi ("ext"),
                              gi ("bass"), gi ("octave") - 1,
                              apvts.getRawParameterValue ("humanize")->load(), seed);

    const juce::ScopedLock sl (eventLock);
    events     = std::move (res.events);
    totalBeats = res.totalBeats;
    displayChords.clear();
    for (auto& d : res.display)
        displayChords.push_back ({ d.first, d.second });
}

std::vector<EX2ChordsProcessor::DisplayChord> EX2ChordsProcessor::getDisplayChords()
{
    rebuildIfNeeded();
    const juce::ScopedLock sl (eventLock);
    return displayChords;
}

void EX2ChordsProcessor::reroll()
{
    auto& rng = juce::Random::getSystemRandom();
    seed = (juce::uint32) rng.nextInt64();

    auto setChoice = [this, &rng] (const char* id, int count)
    {
        if (auto* p = dynamic_cast<juce::AudioParameterChoice*> (apvts.getParameter (id)))
        {
            const float norm = p->convertTo0to1 ((float) rng.nextInt (count));
            p->beginChangeGesture();
            p->setValueNotifyingHost (norm);
            p->endChangeGesture();
        }
    };
    setChoice ("prog",   (int) ex2::progressions().size());
    setChoice ("rhythm", (int) ex2::rhythms().size());
    dirty = true;
}

void EX2ChordsProcessor::sendAllNotesOff (juce::MidiBuffer& midi, int samplePos)
{
    for (const auto& p : pendingOffs)
        midi.addEvent (juce::MidiMessage::noteOff (1, p.note), samplePos);
    pendingOffs.clear();
    midi.addEvent (juce::MidiMessage::allNotesOff (1), samplePos);
}

void EX2ChordsProcessor::scheduleRange (double from, double to, juce::MidiBuffer& midi,
                                        int numSamples, double samplesPerBeat)
{
    auto emitSegment = [&] (double f, double t, double beatsBeforeSegment)
    {
        const juce::ScopedLock sl (eventLock);
        for (const auto& ev : events)
        {
            if (ev.startBeat >= f - 1.0e-6 && ev.startBeat < t - 1.0e-6)
            {
                const double beatIntoBlock = beatsBeforeSegment + (ev.startBeat - f);
                const int at = juce::jlimit (0, juce::jmax (0, numSamples - 1),
                                             (int) (beatIntoBlock * samplesPerBeat));
                for (int note : ev.notes)
                {
                    midi.addEvent (juce::MidiMessage::noteOn (1, note,
                                       (juce::uint8) juce::jlimit (1, 127, (int) (ev.velocity * 127.0f))), at);
                    pendingOffs.push_back ({ note, at + juce::jmax (16, (int) (ev.lengthBeats * samplesPerBeat)) });
                }
            }
        }
    };

    if (to <= totalBeats + 1.0e-9)
    {
        emitSegment (from, to, 0.0);
    }
    else
    {
        emitSegment (from, totalBeats, 0.0);
        emitSegment (0.0, to - totalBeats, totalBeats - from);
    }
}

void EX2ChordsProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    buffer.clear();
    rebuildIfNeeded();

    bool hostPlaying = false;
    double ppq = 0.0, bpm = lastBpm.load();

    if (auto* ph = getPlayHead())
    {
        if (auto pos = ph->getPosition())
        {
            if (auto b = pos->getBpm()) { bpm = *b; lastBpm.store (bpm); }
            hostPlaying = pos->getIsPlaying();
            if (auto p = pos->getPpqPosition()) ppq = *p;
        }
    }

    const bool preview  = apvts.getRawParameterValue ("preview")->load() > 0.5f;
    const bool sendHost = apvts.getRawParameterValue ("hostmidi")->load() > 0.5f;

    const int numSamples = buffer.getNumSamples();
    if (numSamples == 0) return;

    const double spb        = sampleRateHz * 60.0 / juce::jmax (20.0, bpm);
    const double blockBeats = numSamples / spb;
    const bool active       = (hostPlaying && sendHost) || preview;

    if (active)
    {
        double target = (hostPlaying && sendHost)
                            ? std::fmod (juce::jmax (0.0, ppq), totalBeats)
                            : (playCursor < 0.0 ? 0.0 : playCursor);

        if (playCursor >= 0.0)
        {
            double d = std::abs (target - playCursor);
            d = juce::jmin (d, totalBeats - d);
            if (d > 0.3) { sendAllNotesOff (midi, 0); }
        }

        scheduleRange (target, target + blockBeats, midi, numSamples, spb);
        playCursor = std::fmod (target + blockBeats, totalBeats);
    }
    else if (wasActive)
    {
        sendAllNotesOff (midi, 0);
        playCursor = -1.0;
    }
    wasActive = active;

    // flush pending note-offs that fall inside this block
    for (auto it = pendingOffs.begin(); it != pendingOffs.end();)
    {
        if (it->samplesLeft < numSamples)
        {
            midi.addEvent (juce::MidiMessage::noteOff (1, it->note), juce::jmax (0, it->samplesLeft));
            it = pendingOffs.erase (it);
        }
        else
        {
            it->samplesLeft -= numSamples;
            ++it;
        }
    }

    if (previewWas && ! preview)
        synth.allNotesOff (1, false);
    if (preview)
        synth.renderNextBlock (buffer, midi, 0, numSamples);
    previewWas = preview;
}

juce::File EX2ChordsProcessor::writeMidiToTemp()
{
    auto f = juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile ("EX2 Chords.mid");
    return writeMidiTo (f) ? f : juce::File();
}

bool EX2ChordsProcessor::writeMidiTo (const juce::File& f)
{
    rebuildIfNeeded();
    juce::MidiFile mf;
    {
        const juce::ScopedLock sl (eventLock);
        mf = ex2::buildMidiFile (events, lastBpm.load());
    }
    f.deleteFile();
    juce::FileOutputStream os (f);
    if (! os.openedOk())
        return false;
    return mf.writeTo (os);
}

void EX2ChordsProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void EX2ChordsProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        apvts.replaceState (juce::ValueTree::fromXml (*xml));
    dirty = true;
}

juce::AudioProcessorEditor* EX2ChordsProcessor::createEditor()
{
    return new EX2ChordsEditor (*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new EX2ChordsProcessor();
}
