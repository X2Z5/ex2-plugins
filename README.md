# EX2 Plugins

**Two free VST3 plugins for FL Studio (and any VST3 host) — Windows & macOS.**

J-pop chords that bounce, and an Auto-Key-style analyzer that tells you any song's key, BPM, and detune.

> Built with JUCE 8 · VST3 · Windows + macOS (universal: Intel & Apple Silicon)

---

## 🎹 EX2 Chords — J-pop progression & bounce generator

Writes real chord progressions straight into your piano roll.

- **11 progression presets** from Japanese pop and city pop theory: Royal Road (王道進行 4-5-3-6), Komuro 6-4-5-1, Just The Two Of Us changes, Anime Canon, ii-V-I grooves, minor drama and more
- **7 rhythm engines** that make chords *bounce*: Jersey Bounce (the classic club kick grid), Waera Bounce stabs, Pump 8ths, Offbeat Stabs, Halftime, Pads
- **Chord flavor**: simple triads, 7th chords, or lush 9ths — with automatic smooth voice leading between chords
- **Bass modes**: root bass or octave-bounce bass
- **DICE** button rerolls progression + rhythm + feel instantly
- **Preview synth** built in — hear it before you commit
- **Drag & drop MIDI** directly into FL Studio's piano roll (or any DAW), or save as a `.mid` file
- Optional live MIDI output: drive any synth in your project while the transport plays

## 🔍 EX2 Detect — key / BPM / detune analyzer

Drop it on your Master, press play, read the song.

- **Key detection** with confidence meter + relative key (chromagram → Krumhansl-Schmuckler profile matching)
- **Tempo detection** with confidence meter (spectral-flux onsets → autocorrelation)
- **Detune readout**: how far the track sits from standard A440 tuning, in cents, plus the implied reference pitch (e.g. `-31.8 cents → A4 = 432.0 Hz`) — perfect for matching detuned samples
- 100% transparent: audio passes through untouched, safe on the Master bus
- Readings settle after ~15–20 seconds; RESET button when you switch songs

---

## 📥 Download

Grab the latest build from the [**Actions** tab](../../actions): open the newest green "Build VST3 plugins" run and download the artifact for your OS (`EX2-Plugins-Windows` or `EX2-Plugins-macOS`).

## 🔧 Install

**Windows** — unzip, copy both `.vst3` folders to:

```
C:\Program Files\Common Files\VST3
```

**macOS** — unzip, copy both `.vst3` bundles to:

```
/Library/Audio/Plug-Ins/VST3
```

then clear the quarantine flag (they're unsigned):

```bash
sudo xattr -rd com.apple.quarantine "/Library/Audio/Plug-Ins/VST3/EX2 Chords.vst3" "/Library/Audio/Plug-Ins/VST3/EX2 Detect.vst3"
```

**FL Studio**: Options → Manage plugins → Find installed plugins → search "EX2". EX2 Chords appears under Generators, EX2 Detect under Effects.

## 🚀 Quick start

**Chords:** add EX2 Chords to a channel → pick Key + Progression + Bounce → DICE until it slaps → drag the MIDI box onto any synth's piano roll.

**Detect:** drop EX2 Detect on the Master → play the song → read key, BPM and detune off the face.

## 🛠 Build from source

Requires CMake 3.22+ and a C++17 compiler (Visual Studio 2022 / Xcode). JUCE is fetched automatically.

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

Plugins land in `build/plugins/*/EX2*_artefacts/Release/VST3/`. CI builds for both platforms via the included GitHub Actions workflow (`.github/workflows/build.yml`). A console test harness (`test_harness.cpp`) verifies the chord engine and the analyzer against synthetic audio with known key/BPM/detune.

## ⚠️ Notes

- Detection is an estimate — ambiguous intros, heavy drums-only sections, or modal tracks can fool any key detector. Let a full harmonic section play.
- Not affiliated with Image-Line, Antares, or the artist waera — "Waera Bounce" is just a rhythm preset inspired by that style of Japanese club bounce.
- Built on the [JUCE framework](https://juce.com) (fetched at build time, subject to its own license). Plugin code is free to use and modify for personal projects.
