# EX2 Plugins — two VST3 plugins for FL Studio (Windows + Mac)

You get **two plugins**:

| Plugin | What it does |
|---|---|
| **EX2 Chords** | Generates J-pop / city pop / kawaii chord progressions with bounce rhythms (jersey-club style stabs). Pick a progression + bounce, hit DICE, preview it, then **drag the MIDI straight into FL's piano roll**. |
| **EX2 Detect** | Auto-Key style analyzer. Drop it on your **Master**, press play — it shows the song's **key**, **BPM**, and **detune** (how far off A440 the tuning is, in cents, e.g. "A4 = 432.1 Hz"). |

This folder is **source code**. It becomes real `.vst3` files in one of two ways — use Option A, it's much easier.

---

## Option A (recommended): let GitHub build it for you — no coding tools needed

GitHub will compile both the **Windows** and **Mac** versions automatically, for free.

1. Make a free account at **github.com** (skip if you have one).
2. Top-right **+** → **New repository** → name it `ex2-plugins` → keep **Public** → **Create repository**.
3. On the new repo page click **uploading an existing file**, then drag **everything inside this folder** (including the `plugins` folder, `CMakeLists.txt`, etc.) into the upload box → **Commit changes**.
4. Check that the hidden `.github` folder made it: you should see `.github/workflows/build.yml` in the repo. If it's missing (Mac hides dot-folders): click **Add file → Create new file**, type `.github/workflows/build.yml` as the name, paste the contents of that file from this folder, **Commit**.
5. Click the **Actions** tab. A build called "Build VST3 plugins" starts by itself (if not, click it → **Run workflow**). Wait ~10 minutes for the green check.
6. Click the finished run → scroll to **Artifacts** → download **EX2-Plugins-Windows** and/or **EX2-Plugins-macOS**. Each zip contains `EX2 Chords.vst3` and `EX2 Detect.vst3`.

## Option B: build on your own computer

- **Windows:** install Visual Studio Community (with "Desktop development with C++") + CMake. Then in this folder: `cmake -B build -DCMAKE_BUILD_TYPE=Release` then `cmake --build build --config Release`.
- **Mac:** install Xcode + CMake (`brew install cmake`). Same two commands.
- Finished plugins land in `build/plugins/*/EX2*_artefacts/Release/VST3/`.

---

## Installing into FL Studio

**Windows** — copy both `.vst3` folders into:
```
C:\Program Files\Common Files\VST3
```

**Mac** — copy both `.vst3` bundles into:
```
/Library/Audio/Plug-Ins/VST3
```
Then (Mac only) remove the "downloaded from internet" quarantine flag — open Terminal and run:
```
sudo xattr -rd com.apple.quarantine "/Library/Audio/Plug-Ins/VST3/EX2 Chords.vst3" "/Library/Audio/Plug-Ins/VST3/EX2 Detect.vst3"
```

**Then in FL Studio:** Options → Manage plugins → **Find installed plugins**. Search "EX2" in the plugin database — tick the ⭐ to favorite them.

---

## Using EX2 Chords

1. Add **EX2 Chords** to a channel (it appears under Installed → Generators).
2. Pick a **Key**, a **Progression** (Royal Road = the classic J-pop sound), and a **Bounce** (Jersey Bounce / Waera Bounce = club-style stabs).
3. **Chord Flavor**: 7th Chords or Lush 9ths = instant J-pop color. **Bass**: Octave Bounce makes it jump.
4. Hit **DICE** for new combos, **PREVIEW** to hear it with the built-in synth.
5. **Drag the "DRAG MIDI INTO FL" box onto the Piano roll** of any synth (Flex, Sytrus, anything). Done — the chords are yours to edit.
6. Optional live mode: with "MIDI out while host plays" on, set the EX2 Chords wrapper's **MIDI output port** (gear icon → VST wrapper settings) to e.g. `1`, and set your synth's **input port** to `1`. Now EX2 Chords plays that synth whenever the transport runs.

## Using EX2 Detect

1. Drop **EX2 Detect** on the **Master** mixer track (or on the track of one song/sample).
2. Press play. After ~15–20 seconds the readings settle:
   - **KEY** — e.g. "F minor" (+ its relative major, and a confidence bar)
   - **TEMPO** — detected BPM
   - **DETUNE** — cents away from standard A440 tuning, plus the implied reference (e.g. A4 = 432.0 Hz). Use this to nudge your own instruments' master tuning to match a sample.
3. Press **RESET** when you switch to a different song.

Audio passes through untouched — it's safe to leave on the Master.

---

## Troubleshooting

- **FL doesn't find the plugins:** Options → Manage plugins → make sure the VST3 path above is in the search paths → Find installed plugins again.
- **Mac says the plugin can't be opened / is damaged:** run the `xattr` command above (they're unsigned, that's all).
- **GitHub Actions didn't start:** Actions tab → select "Build VST3 plugins" → "Run workflow".
- **Key reading jumps around:** intros with no chords confuse it — let a full section with harmony play, or RESET during the chorus.

Built with the JUCE framework (fetched automatically at build time, licensed under its own terms — fine for personal use; the splash screen on load is JUCE's free-tier requirement).
