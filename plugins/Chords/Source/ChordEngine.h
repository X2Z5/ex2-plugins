#pragma once
// EX2 Chords — chord/rhythm generation engine (pure logic, no audio)
#include <juce_core/juce_core.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <vector>
#include <cmath>
#include <algorithm>

namespace ex2
{

enum class Quality { Maj, Min, Dom7, Maj7, Min7, Min7b5, Sus2, Sus4 };

struct ChordSpec   { int degreeSemis; Quality q; double lengthBars; };
struct Progression { const char* name; std::vector<ChordSpec> chords; };

struct RhythmHit     { double startBeat; double lengthBeats; float accent; };
struct RhythmPattern { const char* name; std::vector<std::vector<RhythmHit>> bars; };

struct ChordEvent
{
    double startBeat = 0.0, lengthBeats = 1.0;
    std::vector<int> notes;
    float velocity = 0.8f;
};

struct GenResult
{
    std::vector<ChordEvent> events;
    std::vector<std::pair<juce::String, double>> display; // chord name, length in bars
    double totalBeats = 16.0;
};

//==============================================================================
inline const std::vector<Progression>& progressions()
{
    static const std::vector<Progression> p = {
        { "Royal Road 4-5-3-6 (J-Pop classic)",
            {{5,Quality::Maj7,1},{7,Quality::Dom7,1},{4,Quality::Min7,1},{9,Quality::Min7,1}} },
        { "Komuro 6-4-5-1 (90s J-Pop)",
            {{9,Quality::Min7,1},{5,Quality::Maj7,1},{7,Quality::Dom7,1},{0,Quality::Maj7,1}} },
        { "Just The Two Of Us (City Pop)",
            {{5,Quality::Maj7,1},{4,Quality::Dom7,1},{9,Quality::Min7,1},{7,Quality::Min7,0.5},{0,Quality::Dom7,0.5}} },
        { "ii-V-I Groove (City Pop)",
            {{2,Quality::Min7,1},{7,Quality::Dom7,1},{0,Quality::Maj7,1},{9,Quality::Min7,1}} },
        { "Anime Canon (8 chords)",
            {{0,Quality::Maj,0.5},{7,Quality::Maj,0.5},{9,Quality::Min,0.5},{4,Quality::Min,0.5},
             {5,Quality::Maj,0.5},{0,Quality::Maj,0.5},{5,Quality::Maj,0.5},{7,Quality::Maj,0.5}} },
        { "Emotional 6-4-1-5",
            {{9,Quality::Min7,1},{5,Quality::Maj7,1},{0,Quality::Maj,1},{7,Quality::Maj,1}} },
        { "Kawaii Lift 1-3-4-5",
            {{0,Quality::Maj7,1},{4,Quality::Min7,1},{5,Quality::Maj7,1},{7,Quality::Dom7,1}} },
        { "Sentimental 4-3-6-5",
            {{5,Quality::Maj7,1},{4,Quality::Min7,1},{9,Quality::Min7,1},{7,Quality::Dom7,1}} },
        { "Future Bounce 4-5-6-6",
            {{5,Quality::Maj7,1},{7,Quality::Dom7,1},{9,Quality::Min7,1},{9,Quality::Min7,1}} },
        { "Minor Drama i-bVI-bIII-bVII",
            {{0,Quality::Min,1},{8,Quality::Maj,1},{3,Quality::Maj,1},{10,Quality::Maj,1}} },
        { "Night Drive i-bVII-bVI-V",
            {{0,Quality::Min7,1},{10,Quality::Maj7,1},{8,Quality::Maj7,1},{7,Quality::Dom7,1}} },
    };
    return p;
}

inline const std::vector<RhythmPattern>& rhythms()
{
    static const std::vector<RhythmPattern> r = {
        { "Hold (pads)",
            {{{0,3.95,1.0f}}} },
        { "Pump 8ths",
            {{{0,.45,1.0f},{.5,.45,.8f},{1,.45,.92f},{1.5,.45,.8f},{2,.45,.96f},{2.5,.45,.8f},{3,.45,.92f},{3.5,.45,.86f}}} },
        { "Jersey Bounce",   // classic club kick grid: X..X..X...X.X...
            {{{0,.55,1.0f},{.75,.4,.86f},{1.5,.55,.95f},{2.5,.4,.9f},{3,.95,1.0f}}} },
        { "Jersey Double",
            {{{0,.55,1.0f},{.75,.4,.86f},{1.5,.55,.95f},{2.5,.4,.9f},{3,.95,1.0f}},
             {{0,.55,1.0f},{.75,.4,.86f},{1.5,.4,.9f},{2,.3,.8f},{2.5,.3,.86f},{3,.45,.95f},{3.5,.45,.9f}}} },
        { "Waera Bounce (stabs)",
            {{{0,.3,1.0f},{.75,.3,.9f},{1.5,.3,.95f},{2.25,.25,.85f},{3,.3,1.0f},{3.5,.3,.9f}}} },
        { "Offbeat Stabs",
            {{{.5,.35,1.0f},{1.5,.35,.9f},{2.5,.35,.95f},{3.5,.35,.9f}}} },
        { "Halftime",
            {{{0,1.6,1.0f},{2.5,1.4,.88f}}} },
    };
    return r;
}

//==============================================================================
inline const char* noteName (int pc)
{
    static const char* names[12] = { "C","C#","D","D#","E","F","F#","G","G#","A","A#","B" };
    return names[((pc % 12) + 12) % 12];
}

inline std::vector<int> intervalsFor (Quality q, int ext) // ext: 0 triads, 1 sevenths, 2 ninths
{
    using Q = Quality;
    auto triad = [] (Q qq) -> std::vector<int>
    {
        switch (qq)
        {
            case Q::Min: case Q::Min7:  return {0,3,7};
            case Q::Min7b5:             return {0,3,6};
            case Q::Sus2:               return {0,2,7};
            case Q::Sus4:               return {0,5,7};
            default:                    return {0,4,7};
        }
    };
    if (ext == 0) return triad (q);
    switch (q)
    {
        case Q::Maj:    return ext == 2 ? std::vector<int>{0,4,7,14}      : triad (q);
        case Q::Min:    return ext == 2 ? std::vector<int>{0,3,7,14}      : triad (q);
        case Q::Maj7:   return ext == 2 ? std::vector<int>{0,4,7,11,14}   : std::vector<int>{0,4,7,11};
        case Q::Min7:   return ext == 2 ? std::vector<int>{0,3,7,10,14}   : std::vector<int>{0,3,7,10};
        case Q::Dom7:   return ext == 2 ? std::vector<int>{0,4,7,10,14}   : std::vector<int>{0,4,7,10};
        case Q::Min7b5: return {0,3,6,10};
        case Q::Sus2:   return {0,2,7};
        case Q::Sus4:   return {0,5,7,10};
    }
    return triad (q);
}

inline juce::String qualitySuffix (Quality q, int ext)
{
    using Q = Quality;
    if (ext == 0)
        switch (q)
        {
            case Q::Min: case Q::Min7: return "m";
            case Q::Min7b5:            return "dim";
            case Q::Sus2:              return "sus2";
            case Q::Sus4:              return "sus4";
            default:                   return "";
        }
    switch (q)
    {
        case Q::Maj:    return ext == 2 ? "add9" : "";
        case Q::Min:    return ext == 2 ? "m(add9)" : "m";
        case Q::Maj7:   return ext == 2 ? "maj9" : "maj7";
        case Q::Min7:   return ext == 2 ? "m9"   : "m7";
        case Q::Dom7:   return ext == 2 ? "9"    : "7";
        case Q::Min7b5: return "m7b5";
        case Q::Sus2:   return "sus2";
        case Q::Sus4:   return "7sus4";
    }
    return "";
}

// Choose an inversion/register that moves smoothly from the previous chord.
inline std::vector<int> voiceChord (int rootPc, const std::vector<int>& iv,
                                    const std::vector<int>& prev, int octave)
{
    const int base = 48 + rootPc + octave * 12;
    std::vector<int> best;
    double bestCost = 1.0e9;

    for (size_t inv = 0; inv < iv.size(); ++inv)
    {
        for (int shift : { -12, 0, 12 })
        {
            std::vector<int> cand;
            cand.reserve (iv.size());
            for (size_t i = 0; i < iv.size(); ++i)
                cand.push_back (base + iv[i] + shift + (i < inv ? 12 : 0));
            std::sort (cand.begin(), cand.end());

            double cost = 0.0;
            for (int v : cand)
            {
                if (v < 43) cost += (43 - v) * 2.0;
                if (v > 79) cost += (v - 79) * 2.0;
            }
            if (prev.empty())
            {
                double mean = 0; for (int v : cand) mean += v;
                mean /= (double) cand.size();
                cost += std::abs (mean - (60.0 + octave * 12.0));
            }
            else
            {
                const size_t m = std::min (prev.size(), cand.size());
                for (size_t i = 0; i < m; ++i)
                    cost += std::abs ((double) cand[i] - (double) prev[i]) * 0.6;
                double mp = 0, mc = 0;
                for (int v : prev) mp += v;  mp /= (double) prev.size();
                for (int v : cand) mc += v;  mc /= (double) cand.size();
                cost += std::abs (mc - mp) * 1.2;
            }
            if (cost < bestCost) { bestCost = cost; best = cand; }
        }
    }
    return best;
}

//==============================================================================
inline GenResult generate (int key, int progIdx, int rhythmIdx, int ext,
                           int bassMode, int octave, float humanize, juce::uint32 seed)
{
    juce::Random rng ((juce::int64) seed);
    const auto& prog = progressions()[(size_t) juce::jlimit (0, (int) progressions().size() - 1, progIdx)];
    const auto& rhy  = rhythms()[(size_t) juce::jlimit (0, (int) rhythms().size() - 1, rhythmIdx)];

    GenResult out;
    double cursor = 0.0;
    std::vector<int> prevVoicing;

    for (const auto& cs : prog.chords)
    {
        const double lenBeats = cs.lengthBars * 4.0;
        const double t0 = cursor, t1 = cursor + lenBeats;

        const int rootPc = ((key + cs.degreeSemis) % 12 + 12) % 12;
        const auto iv     = intervalsFor (cs.q, ext);
        const auto voiced = voiceChord (rootPc, iv, prevVoicing, octave);
        prevVoicing = voiced;

        out.display.push_back ({ juce::String (noteName (rootPc)) + qualitySuffix (cs.q, ext), cs.lengthBars });

        const int firstBar = (int) std::floor (t0 / 4.0 + 1.0e-6);
        const int lastBar  = (int) std::floor ((t1 - 1.0e-3) / 4.0);
        for (int b = firstBar; b <= lastBar; ++b)
        {
            const auto& barPat = rhy.bars[(size_t) (b % (int) rhy.bars.size())];
            for (const auto& h : barPat)
            {
                const double hs = b * 4.0 + h.startBeat;
                if (hs < t0 - 1.0e-4 || hs >= t1 - 1.0e-4) continue;

                ChordEvent ev;
                ev.startBeat   = hs;
                ev.lengthBeats = std::max (0.1, std::min ((double) h.lengthBeats, t1 - hs - 0.03));
                ev.notes       = voiced;
                const float jit = 1.0f + humanize * 0.18f * (rng.nextFloat() * 2.0f - 1.0f);
                ev.velocity    = juce::jlimit (0.15f, 1.0f, 0.78f * h.accent * jit);
                out.events.push_back (std::move (ev));
            }
        }

        if (bassMode > 0)
        {
            const int bassNote = 36 + rootPc;
            if (bassMode == 1)
            {
                ChordEvent ev;
                ev.startBeat = t0; ev.lengthBeats = lenBeats - 0.05;
                ev.notes = { bassNote }; ev.velocity = 0.85f;
                out.events.push_back (std::move (ev));
            }
            else // octave bounce 8ths
            {
                int k = 0;
                for (double t = t0; t < t1 - 1.0e-4; t += 0.5, ++k)
                {
                    ChordEvent ev;
                    ev.startBeat = t; ev.lengthBeats = 0.42;
                    ev.notes = { (k % 2 == 0) ? bassNote : bassNote + 12 };
                    const float jit = 1.0f + humanize * 0.15f * (rng.nextFloat() * 2.0f - 1.0f);
                    ev.velocity = juce::jlimit (0.15f, 1.0f, ((k % 2 == 0) ? 0.9f : 0.72f) * jit);
                    out.events.push_back (std::move (ev));
                }
            }
        }
        cursor = t1;
    }
    out.totalBeats = std::max (4.0, cursor);
    return out;
}

inline juce::MidiFile buildMidiFile (const std::vector<ChordEvent>& evs, double bpm)
{
    const int ppq = 960;
    juce::MidiMessageSequence seq;
    auto tempo = juce::MidiMessage::tempoMetaEvent ((int) std::round (60000000.0 / juce::jmax (20.0, bpm)));
    tempo.setTimeStamp (0.0);
    seq.addEvent (tempo);

    for (const auto& ev : evs)
        for (int note : ev.notes)
        {
            auto on  = juce::MidiMessage::noteOn  (1, note, (juce::uint8) juce::jlimit (1, 127, (int) (ev.velocity * 127.0f)));
            on.setTimeStamp (ev.startBeat * ppq);
            seq.addEvent (on);
            auto off = juce::MidiMessage::noteOff (1, note);
            off.setTimeStamp ((ev.startBeat + ev.lengthBeats) * ppq);
            seq.addEvent (off);
        }

    seq.sort();
    seq.updateMatchedPairs();
    juce::MidiFile mf;
    mf.setTicksPerQuarterNote (ppq);
    mf.addTrack (seq);
    return mf;
}

//==============================================================================
// Tiny preview synth
struct AnySound : public juce::SynthesiserSound
{
    bool appliesToNote (int) override    { return true; }
    bool appliesToChannel (int) override { return true; }
};

struct SawVoice : public juce::SynthesiserVoice
{
    bool canPlaySound (juce::SynthesiserSound*) override { return true; }

    void startNote (int note, float vel, juce::SynthesiserSound*, int) override
    {
        level = 0.16f * (0.4f + 0.6f * vel);
        const double f = juce::MidiMessage::getMidiNoteInHertz (note);
        delta1 = f / getSampleRate();
        delta2 = f * 1.003 / getSampleRate();
        phase1 = phase2 = 0.0; lp = 0.0f;
        adsr.setSampleRate (getSampleRate());
        adsr.setParameters ({ 0.004f, 0.16f, 0.6f, 0.14f });
        adsr.noteOn();
    }
    void stopNote (float, bool allowTail) override
    {
        adsr.noteOff();
        if (! allowTail) clearCurrentNote();
    }
    void pitchWheelMoved (int) override {}
    void controllerMoved (int, int) override {}

    void renderNextBlock (juce::AudioBuffer<float>& out, int start, int num) override
    {
        if (! adsr.isActive()) { clearCurrentNote(); return; }
        for (int i = 0; i < num; ++i)
        {
            const float s = (float) (phase1 * 2.0 - 1.0) * 0.6f + (float) (phase2 * 2.0 - 1.0) * 0.4f;
            phase1 += delta1; if (phase1 >= 1.0) phase1 -= 1.0;
            phase2 += delta2; if (phase2 >= 1.0) phase2 -= 1.0;
            lp += 0.22f * (s - lp);
            const float v = lp * level * adsr.getNextSample();
            for (int c = 0; c < out.getNumChannels(); ++c)
                out.addSample (c, start + i, v);
        }
        if (! adsr.isActive()) clearCurrentNote();
    }

    double phase1 = 0, phase2 = 0, delta1 = 0, delta2 = 0;
    float lp = 0.0f, level = 0.15f;
    juce::ADSR adsr;
};

} // namespace ex2
