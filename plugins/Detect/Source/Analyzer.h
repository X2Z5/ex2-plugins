#pragma once
// EX2 Detect — background analysis engine: key, BPM, tuning offset (detune)
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>
#include <atomic>
#include <vector>
#include <cmath>
#include <algorithm>

namespace ex2
{

class Analyzer : private juce::Thread
{
public:
    Analyzer() : juce::Thread ("EX2DetectAnalysis"), fftOnset (10), fftSpec (13)
    {
        winOnset.resize (1024);
        winSpec.resize (4096);
        const double pi = juce::MathConstants<double>::pi;
        for (int i = 0; i < 1024; ++i) winOnset[(size_t) i] = (float) (0.5 * (1.0 - std::cos (2.0 * pi * i / 1023.0)));
        for (int i = 0; i < 4096; ++i) winSpec[(size_t) i]  = (float) (0.5 * (1.0 - std::cos (2.0 * pi * i / 4095.0)));
        fftBufO.resize (2048, 0.0f);
        fftBufS.resize (16384, 0.0f);
        prevMag.resize (513, 0.0f);
        startThread();
    }

    ~Analyzer() override { stopThread (3000); }

    void prepare (double sr)
    {
        sampleRate.store (sr);
        requestReset.store (true);
    }

    void reset() { requestReset.store (true); }

    // called from the audio thread
    void push (const float* data, int n)
    {
        int s1, n1, s2, n2;
        fifo.prepareToWrite (n, s1, n1, s2, n2);
        for (int i = 0; i < n1; ++i) ring[(size_t) (s1 + i)] = data[i];
        for (int i = 0; i < n2; ++i) ring[(size_t) (s2 + i)] = data[n1 + i];
        fifo.finishedWrite (n1 + n2);
        notify();
    }

    // results (read by the UI)
    std::atomic<float> bpm     { 0.0f },  bpmConf { 0.0f };
    std::atomic<float> cents   { 0.0f };                    // tuning offset from A440, -50..+50
    std::atomic<float> level   { 0.0f };
    std::atomic<float> keyConf { 0.0f };
    std::atomic<int>   keyIndex { -1 };                     // 0..11 major C..B, 12..23 minor C..B
    std::atomic<int>   specFrames { 0 };

private:
    void run() override
    {
        while (! threadShouldExit())
        {
            wait (40);
            if (requestReset.exchange (false))
                clearAll();

            const int ready = fifo.getNumReady();
            if (ready > 0)
            {
                int s1, n1, s2, n2;
                fifo.prepareToRead (ready, s1, n1, s2, n2);
                appendTo (s1, n1);
                appendTo (s2, n2);
                fifo.finishedRead (n1 + n2);
            }
            processOnsetFrames();
            processSpectralFrames();
        }
    }

    void appendTo (int start, int num)
    {
        if (num <= 0) return;
        onsetBuf.insert (onsetBuf.end(), ring.begin() + start, ring.begin() + start + num);
        specBuf.insert  (specBuf.end(),  ring.begin() + start, ring.begin() + start + num);
    }

    void clearAll()
    {
        onsetBuf.clear(); specBuf.clear(); fluxRing.clear(); bpmHist.clear();
        std::fill (prevMag.begin(), prevMag.end(), 0.0f);
        for (auto& c : chroma) c = 0.0;
        tunSx = tunSy = 0.0;
        fluxCount = 0;
        bpm.store (0.0f); bpmConf.store (0.0f);
        keyIndex.store (-1); keyConf.store (0.0f);
        cents.store (0.0f); specFrames.store (0);
    }

    //==========================================================================
    void processOnsetFrames()
    {
        const double sr = sampleRate.load();
        const int maxFluxLen = (int) (20.0 * sr / 256.0);

        while ((int) onsetBuf.size() >= 1024)
        {
            for (int i = 0; i < 1024; ++i)
                fftBufO[(size_t) i] = onsetBuf[(size_t) i] * winOnset[(size_t) i];
            std::fill (fftBufO.begin() + 1024, fftBufO.end(), 0.0f);
            fftOnset.performFrequencyOnlyForwardTransform (fftBufO.data());

            float flux = 0.0f;
            for (int k = 1; k <= 512; ++k)
            {
                const float d = fftBufO[(size_t) k] - prevMag[(size_t) k];
                if (d > 0.0f) flux += d;
                prevMag[(size_t) k] = fftBufO[(size_t) k];
            }
            fluxRing.push_back (flux);
            if ((int) fluxRing.size() > maxFluxLen)
                fluxRing.erase (fluxRing.begin(), fluxRing.begin() + ((int) fluxRing.size() - maxFluxLen));

            onsetBuf.erase (onsetBuf.begin(), onsetBuf.begin() + 256);

            if (++fluxCount % 32 == 0)
                estimateBpm();
        }
    }

    void estimateBpm()
    {
        const double sr = sampleRate.load();
        const double envRate = sr / 256.0;
        const int N = (int) fluxRing.size();
        if (N < (int) (8.0 * envRate))
            return;

        const int M = std::min (N, (int) (16.0 * envRate));
        std::vector<float> x (fluxRing.end() - M, fluxRing.end());
        float mean = 0.0f;
        for (float v : x) mean += v;
        mean /= (float) M;
        for (auto& v : x) v -= mean;

        double e0 = 0.0;
        for (float v : x) e0 += (double) v * v;
        if (e0 <= 1.0e-9) return;

        const int minLag = std::max (2, (int) (envRate * 60.0 / 200.0));
        const int maxLag = std::min (M / 2, (int) (envRate * 60.0 / 60.0));
        if (maxLag <= minLag) return;

        std::vector<double> ac ((size_t) maxLag + 1, 0.0);
        for (int lag = minLag; lag <= maxLag; ++lag)
        {
            double s = 0.0;
            for (int i = lag; i < M; ++i)
                s += (double) x[(size_t) i] * x[(size_t) (i - lag)];
            ac[(size_t) lag] = s / e0;
        }

        auto acAt = [&] (double lag) -> double
        {
            const int l = (int) std::round (lag);
            return (l >= minLag && l <= maxLag) ? ac[(size_t) l] : 0.0;
        };

        double bestScore = -1.0, bestLag = 0.0;
        for (int lag = minLag; lag <= maxLag; ++lag)
        {
            const double sc = ac[(size_t) lag] + 0.5 * acAt (lag * 2.0) + 0.33 * acAt (lag * 3.0);
            if (sc > bestScore) { bestScore = sc; bestLag = lag; }
        }
        if (bestLag <= 0.0) return;

        const int bl = (int) bestLag;
        if (bl > minLag && bl < maxLag)
        {
            const double a = ac[(size_t) (bl - 1)], b = ac[(size_t) bl], c = ac[(size_t) (bl + 1)];
            const double den = a - 2.0 * b + c;
            if (std::abs (den) > 1.0e-12)
                bestLag = bl + 0.5 * (a - c) / den;
        }

        double cand = 60.0 * envRate / bestLag;
        auto strength = [&] (double bpmV) { return acAt (60.0 * envRate / bpmV); };

        double best = cand, bs = (cand >= 68.0 && cand <= 185.0) ? strength (cand) : -1.0e9;
        for (double m : { 0.5, 2.0 })
        {
            const double v = cand * m;
            if (v >= 68.0 && v <= 185.0)
            {
                const double s = strength (v) + ((v >= 85.0 && v <= 165.0) ? 0.02 : 0.0);
                if (s > bs) { bs = s; best = v; }
            }
        }
        while (best < 68.0)  best *= 2.0;
        while (best > 185.0) best /= 2.0;

        bpmHist.push_back ((float) best);
        if (bpmHist.size() > 7)
            bpmHist.erase (bpmHist.begin());
        auto sorted = bpmHist;
        std::sort (sorted.begin(), sorted.end());
        const float med = sorted[sorted.size() / 2];

        bpm.store (med);
        bpmConf.store ((float) juce::jlimit (0.0, 1.0, strength ((double) med) * 1.6));
    }

    //==========================================================================
    void processSpectralFrames()
    {
        const double sr = sampleRate.load();

        while ((int) specBuf.size() >= 4096)
        {
            double rms = 0.0;
            for (int i = 0; i < 4096; ++i)
                rms += (double) specBuf[(size_t) i] * specBuf[(size_t) i];
            rms = std::sqrt (rms / 4096.0);
            level.store ((float) rms);

            if (rms > 1.0e-4)  // skip near-silence
            {
                for (int i = 0; i < 4096; ++i)
                    fftBufS[(size_t) i] = specBuf[(size_t) i] * winSpec[(size_t) i];
                std::fill (fftBufS.begin() + 4096, fftBufS.end(), 0.0f);
                fftSpec.performFrequencyOnlyForwardTransform (fftBufS.data());

                const double binHz = sr / 8192.0;
                const int kLo = std::max (3, (int) (55.0 / binHz));
                const int kHi = std::min (4094, (int) (4500.0 / binHz));

                // collect strongest local maxima
                struct Peak { float mag; int k; };
                std::vector<Peak> peaks;
                for (int k = kLo; k <= kHi; ++k)
                {
                    const float m = fftBufS[(size_t) k];
                    if (m > fftBufS[(size_t) (k - 1)] && m >= fftBufS[(size_t) (k + 1)] && m > 1.0e-5f)
                        peaks.push_back ({ m, k });
                }
                const size_t topN = 24;
                if (peaks.size() > topN)
                {
                    std::partial_sort (peaks.begin(), peaks.begin() + (long) topN, peaks.end(),
                                       [] (const Peak& a, const Peak& b) { return a.mag > b.mag; });
                    peaks.resize (topN);
                }

                // decay accumulators
                tunSx *= 0.995; tunSy *= 0.995;
                for (auto& c : chroma) c *= 0.995;

                const double curDev = (specFrames.load() > 30) ? (double) cents.load() : 0.0;
                const double pi = juce::MathConstants<double>::pi;

                for (const auto& pk : peaks)
                {
                    const double a = fftBufS[(size_t) (pk.k - 1)], b = pk.mag, c2 = fftBufS[(size_t) (pk.k + 1)];
                    const double den = a - 2.0 * b + c2;
                    const double p = (std::abs (den) > 1.0e-12) ? 0.5 * (a - c2) / den : 0.0;
                    const double f = ((double) pk.k + juce::jlimit (-0.5, 0.5, p)) * binHz;
                    if (f < 50.0) continue;

                    const double midi = 69.0 + 12.0 * std::log2 (f / 440.0);
                    const double dev  = (midi - std::round (midi)) * 100.0;   // -50..+50 cents
                    const double w    = std::sqrt ((double) pk.mag);

                    const double ang = dev / 50.0 * pi;
                    tunSx += w * std::cos (ang);
                    tunSy += w * std::sin (ang);

                    int pc = (int) std::lround (midi - curDev / 100.0) % 12;
                    pc = (pc % 12 + 12) % 12;
                    chroma[(size_t) pc] += w;
                }

                if (std::abs (tunSx) > 1.0e-9 || std::abs (tunSy) > 1.0e-9)
                    cents.store ((float) (std::atan2 (tunSy, tunSx) / pi * 50.0));

                const int fr = specFrames.fetch_add (1) + 1;
                if (fr % 8 == 0)
                    estimateKey();
            }

            specBuf.erase (specBuf.begin(), specBuf.begin() + 4096);
        }
    }

    void estimateKey()
    {
        // Krumhansl-Kessler key profiles
        static const double majP[12] = { 6.35,2.23,3.48,2.33,4.38,4.09,2.52,5.19,2.39,3.66,2.29,2.88 };
        static const double minP[12] = { 6.33,2.68,3.52,5.38,2.60,3.53,2.54,4.75,3.98,2.69,3.34,3.17 };

        double c[12];
        double mean = 0.0;
        for (int i = 0; i < 12; ++i) { c[i] = chroma[(size_t) i]; mean += c[i]; }
        mean /= 12.0;
        double var = 0.0;
        for (int i = 0; i < 12; ++i) { c[i] -= mean; var += c[i] * c[i]; }
        if (var < 1.0e-12) return;
        const double sd = std::sqrt (var / 12.0);

        auto corrWith = [&] (const double* prof, int rot) -> double
        {
            double pm = 0.0;
            for (int i = 0; i < 12; ++i) pm += prof[i];
            pm /= 12.0;
            double num = 0.0, pv = 0.0;
            for (int i = 0; i < 12; ++i)
            {
                const double pVal = prof[((i - rot) % 12 + 12) % 12] - pm;
                num += c[i] * pVal;
                pv  += pVal * pVal;
            }
            return num / (12.0 * sd * std::sqrt (pv / 12.0));
        };

        double best = -2.0, second = -2.0;
        int bestIdx = -1;
        for (int rot = 0; rot < 12; ++rot)
        {
            const double cm = corrWith (majP, rot);
            const double cn = corrWith (minP, rot);
            if (cm > best)   { second = best; best = cm; bestIdx = rot; }
            else if (cm > second) second = cm;
            if (cn > best)   { second = best; best = cn; bestIdx = 12 + rot; }
            else if (cn > second) second = cn;
        }
        if (bestIdx < 0) return;

        keyIndex.store (bestIdx);
        const double conf = juce::jlimit (0.0, 1.0, std::max (0.0, best) * 0.6 + std::max (0.0, best - second) * 2.0 * 0.4);
        keyConf.store ((float) conf);
    }

    //==========================================================================
    juce::AbstractFifo fifo { 1 << 16 };
    std::vector<float> ring = std::vector<float> (1 << 16, 0.0f);
    std::atomic<double> sampleRate { 44100.0 };
    std::atomic<bool> requestReset { false };

    std::vector<float> onsetBuf, specBuf, prevMag, fluxRing, bpmHist;
    juce::dsp::FFT fftOnset, fftSpec;
    std::vector<float> winOnset, winSpec, fftBufO, fftBufS;
    double chroma[12] = {};
    double tunSx = 0.0, tunSy = 0.0;
    int fluxCount = 0;
};

inline juce::String keyIndexToName (int idx)
{
    static const char* names[12] = { "C","C#","D","D#","E","F","F#","G","G#","A","A#","B" };
    if (idx < 0) return "--";
    if (idx < 12) return juce::String (names[idx]) + " major";
    return juce::String (names[idx - 12]) + " minor";
}

inline juce::String relativeKeyName (int idx)
{
    static const char* names[12] = { "C","C#","D","D#","E","F","F#","G","G#","A","A#","B" };
    if (idx < 0) return {};
    if (idx < 12) return juce::String (names[(idx + 9) % 12]) + " minor";
    return juce::String (names[((idx - 12) + 3) % 12]) + " major";
}

} // namespace ex2
