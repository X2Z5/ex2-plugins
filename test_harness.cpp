// Console test harness (not part of the plugins) — verifies engine + analyzer logic.
#include "plugins/Chords/Source/ChordEngine.h"
#include "plugins/Detect/Source/Analyzer.h"
#include <cstdio>
#include <cstdlib>

int main()
{
    // ---- 1) Chord engine ----
    for (size_t pi = 0; pi < ex2::progressions().size(); ++pi)
        for (size_t ri = 0; ri < ex2::rhythms().size(); ++ri)
        {
            auto r = ex2::generate (9, (int) pi, (int) ri, 1, 2, 0, 0.35f, 777);
            if (r.events.empty() || r.totalBeats < 4.0) { printf ("FAIL prog=%zu rhy=%zu\n", pi, ri); return 1; }
            for (auto& ev : r.events)
                for (int n : ev.notes)
                    if (n < 20 || n > 100) { printf ("FAIL note range %d\n", n); return 1; }
        }

    auto res = ex2::generate (9 /*A*/, 0 /*Royal Road*/, 2 /*Jersey*/, 1, 2, 0, 0.35f, 12345);
    printf ("engine: events=%d totalBeats=%.1f\n", (int) res.events.size(), res.totalBeats);
    for (auto& d : res.display) printf ("  chord: %-8s %.1f bars\n", d.first.toRawUTF8(), d.second);

    auto mf = ex2::buildMidiFile (res.events, 140.0);
    juce::MemoryOutputStream mos;
    const bool ok = mf.writeTo (mos);
    printf ("midi: ok=%d bytes=%d\n", (int) ok, (int) mos.getDataSize());
    if (! ok || mos.getDataSize() < 100) return 1;

    // ---- 2) Analyzer: A minor arpeggio/chord stabs @ 140 BPM, detuned -30 cents ----
    ex2::Analyzer an;
    an.prepare (44100.0);
    const double sr = 44100.0, bpm = 140.0, detune = -30.0;
    const double tune = std::pow (2.0, detune / 1200.0);
    const double freqs[] = { 110.0, 220.0, 261.63, 329.63, 440.0, 523.25 }; // A,C,E stack
    const double spb = sr * 60.0 / bpm;
    const double pi2 = 2.0 * juce::MathConstants<double>::pi;

    const int total = (int) (sr * 30.0);
    std::vector<float> sig ((size_t) total);
    for (int i = 0; i < total; ++i)
    {
        const double t = i / sr;
        const double beatPos = std::fmod (i / spb, 1.0);
        const double env = std::exp (-beatPos * 5.0);
        double v = 0.0;
        for (double f : freqs) v += std::sin (pi2 * f * tune * t);
        const double burst = (beatPos < 0.04) ? ((rand() % 2000) / 1000.0 - 1.0) : 0.0;
        sig[(size_t) i] = (float) (v * 0.12 * env + burst * 0.8 * std::exp (-beatPos * 40.0));
    }
    for (int i = 0; i < total; i += 16384)
    {
        an.push (sig.data() + i, std::min (16384, total - i));
        juce::Thread::sleep (25);
    }
    juce::Thread::sleep (2500);

    printf ("analyzer: key=%s conf=%.2f | bpm=%.1f conf=%.2f | cents=%.1f | frames=%d\n",
            ex2::keyIndexToName (an.keyIndex.load()).toRawUTF8(), an.keyConf.load(),
            an.bpm.load(), an.bpmConf.load(), an.cents.load(), an.specFrames.load());

    bool pass = true;
    if (ex2::keyIndexToName (an.keyIndex.load()) != "A minor") { printf ("KEY MISMATCH\n"); pass = false; }
    if (std::abs (an.bpm.load() - 140.0f) > 1.5f)              { printf ("BPM MISMATCH\n"); pass = false; }
    if (std::abs (an.cents.load() - (-30.0f)) > 6.0f)          { printf ("CENTS MISMATCH\n"); pass = false; }
    printf (pass ? "ALL TESTS PASSED\n" : "TESTS FAILED\n");
    return pass ? 0 : 1;
}
