#include "PluginEditor.h"

static const juce::Colour kBg1   (0xff10131f);
static const juce::Colour kBg2   (0xff1a2233);
static const juce::Colour kCyan  (0xff59e3ff);
static const juce::Colour kPink  (0xffff5fa2);
static const juce::Colour kGreen (0xff7dffb0);
static const juce::Colour kPanel (0xff1d2434);

EX2DetectEditor::EX2DetectEditor (EX2DetectProcessor& p)
    : juce::AudioProcessorEditor (p), proc (p)
{
    resetBtn.setColour (juce::TextButton::buttonColourId, kPanel);
    resetBtn.setColour (juce::TextButton::textColourOffId, juce::Colours::white);
    resetBtn.onClick = [this] { proc.analyzer.reset(); };
    addAndMakeVisible (resetBtn);

    setSize (560, 380);
    startTimerHz (10);
}

void EX2DetectEditor::resized()
{
    resetBtn.setBounds (getWidth() - 100, 18, 80, 28);
}

static void drawPanel (juce::Graphics& g, juce::Rectangle<float> r, juce::Colour accent)
{
    g.setColour (kPanel);
    g.fillRoundedRectangle (r, 12.0f);
    g.setColour (accent.withAlpha (0.4f));
    g.drawRoundedRectangle (r, 12.0f, 1.2f);
}

void EX2DetectEditor::paint (juce::Graphics& g)
{
    g.setGradientFill (juce::ColourGradient (kBg1, 0, 0, kBg2, (float) getWidth(), (float) getHeight(), false));
    g.fillAll();

    auto& a = proc.analyzer;
    const int   keyIdx  = a.keyIndex.load();
    const float keyCf   = a.keyConf.load();
    const float bpmV    = a.bpm.load();
    const float bpmCf   = a.bpmConf.load();
    const float cent    = a.cents.load();
    const float lvl     = a.level.load();
    const int   frames  = a.specFrames.load();

    g.setColour (kCyan);
    g.setFont (juce::Font (juce::FontOptions (juce::Font::getDefaultSansSerifFontName(), 26.0f, juce::Font::bold)));
    g.drawText ("EX2 DETECT", 24, 14, 280, 30, juce::Justification::left);
    g.setColour (juce::Colours::white.withAlpha (0.55f));
    g.setFont (juce::Font (juce::FontOptions (13.0f)));
    g.drawText ("key  /  bpm  /  detune  —  drop on your Master and press play",
                24, 42, 460, 18, juce::Justification::left);

    // listening dot
    const bool hearing = lvl > 1.0e-4f;
    g.setColour (hearing ? kGreen : juce::Colours::grey);
    g.fillEllipse ((float) getWidth() - 124.0f, 24.0f, 10.0f, 10.0f);

    const float W = (float) getWidth();
    auto keyPanel = juce::Rectangle<float> (24,  86, W - 48, 110);
    auto bpmPanel = juce::Rectangle<float> (24, 210, (W - 60) / 2.0f, 120);
    auto detPanel = juce::Rectangle<float> (36 + (W - 60) / 2.0f, 210, (W - 60) / 2.0f, 120);

    // ---- KEY ----
    drawPanel (g, keyPanel, kPink);
    g.setColour (juce::Colours::white.withAlpha (0.55f));
    g.setFont (juce::Font (juce::FontOptions (12.0f)));
    g.drawText ("KEY", keyPanel.toNearestInt().reduced (16, 10), juce::Justification::topLeft);

    const bool warm = frames > 20;
    g.setColour (juce::Colours::white);
    g.setFont (juce::Font (juce::FontOptions (juce::Font::getDefaultSansSerifFontName(), 44.0f, juce::Font::bold)));
    g.drawText (warm ? ex2::keyIndexToName (keyIdx) : juce::String ("listening..."),
                keyPanel.toNearestInt().reduced (16, 0), juce::Justification::centred);

    if (warm && keyIdx >= 0)
    {
        g.setColour (juce::Colours::white.withAlpha (0.5f));
        g.setFont (juce::Font (juce::FontOptions (14.0f)));
        g.drawText ("relative: " + ex2::relativeKeyName (keyIdx),
                    keyPanel.toNearestInt().reduced (16, 10), juce::Justification::bottomLeft);

        auto conf = juce::Rectangle<float> (keyPanel.getRight() - 136, keyPanel.getBottom() - 24, 120, 8);
        g.setColour (juce::Colours::white.withAlpha (0.12f));
        g.fillRoundedRectangle (conf, 4.0f);
        g.setColour (kPink);
        g.fillRoundedRectangle (conf.withWidth (conf.getWidth() * juce::jlimit (0.0f, 1.0f, keyCf)), 4.0f);
    }

    // ---- BPM ----
    drawPanel (g, bpmPanel, kCyan);
    g.setColour (juce::Colours::white.withAlpha (0.55f));
    g.setFont (juce::Font (juce::FontOptions (12.0f)));
    g.drawText ("TEMPO", bpmPanel.toNearestInt().reduced (16, 10), juce::Justification::topLeft);

    g.setColour (juce::Colours::white);
    g.setFont (juce::Font (juce::FontOptions (juce::Font::getDefaultSansSerifFontName(), 40.0f, juce::Font::bold)));
    g.drawText (bpmV > 1.0f ? juce::String (bpmV, 1) : juce::String ("--"),
                bpmPanel.toNearestInt().withTrimmedBottom (24), juce::Justification::centred);
    g.setColour (juce::Colours::white.withAlpha (0.5f));
    g.setFont (juce::Font (juce::FontOptions (13.0f)));
    g.drawText ("BPM", bpmPanel.toNearestInt().reduced (0, 14), juce::Justification::centredBottom);

    if (bpmV > 1.0f)
    {
        auto conf = juce::Rectangle<float> (bpmPanel.getX() + 16, bpmPanel.getBottom() - 14, bpmPanel.getWidth() - 32, 6);
        g.setColour (juce::Colours::white.withAlpha (0.12f));
        g.fillRoundedRectangle (conf, 3.0f);
        g.setColour (kCyan);
        g.fillRoundedRectangle (conf.withWidth (conf.getWidth() * juce::jlimit (0.0f, 1.0f, bpmCf)), 3.0f);
    }

    // ---- DETUNE ----
    drawPanel (g, detPanel, kGreen);
    g.setColour (juce::Colours::white.withAlpha (0.55f));
    g.setFont (juce::Font (juce::FontOptions (12.0f)));
    g.drawText ("DETUNE (vs A440)", detPanel.toNearestInt().reduced (16, 10), juce::Justification::topLeft);

    if (frames > 20)
    {
        g.setColour (juce::Colours::white);
        g.setFont (juce::Font (juce::FontOptions (juce::Font::getDefaultSansSerifFontName(), 30.0f, juce::Font::bold)));
        g.drawText (juce::String (cent > 0 ? "+" : "") + juce::String (cent, 1) + " cents",
                    detPanel.toNearestInt().withTrimmedBottom (30), juce::Justification::centred);

        const double refHz = 440.0 * std::pow (2.0, (double) cent / 1200.0);
        g.setColour (juce::Colours::white.withAlpha (0.5f));
        g.setFont (juce::Font (juce::FontOptions (13.0f)));
        g.drawText ("A4 = " + juce::String (refHz, 1) + " Hz",
                    detPanel.toNearestInt().reduced (0, 30), juce::Justification::centredBottom);

        // meter -50..+50
        auto m = juce::Rectangle<float> (detPanel.getX() + 16, detPanel.getBottom() - 16, detPanel.getWidth() - 32, 8);
        g.setColour (juce::Colours::white.withAlpha (0.12f));
        g.fillRoundedRectangle (m, 4.0f);
        g.setColour (juce::Colours::white.withAlpha (0.35f));
        g.fillRect (m.getCentreX() - 0.7f, m.getY() - 2.0f, 1.4f, m.getHeight() + 4.0f);
        const float t = juce::jlimit (-50.0f, 50.0f, cent) / 100.0f + 0.5f;
        g.setColour (kGreen);
        g.fillEllipse (m.getX() + t * m.getWidth() - 5.0f, m.getCentreY() - 5.0f, 10.0f, 10.0f);
    }
    else
    {
        g.setColour (juce::Colours::white.withAlpha (0.4f));
        g.setFont (juce::Font (juce::FontOptions (15.0f)));
        g.drawText ("listening...", detPanel.toNearestInt(), juce::Justification::centred);
    }

    g.setColour (juce::Colours::white.withAlpha (0.35f));
    g.setFont (juce::Font (juce::FontOptions (12.0f)));
    g.drawText ("Tip: let 15-20 seconds of the song play for a stable reading. RESET when you switch songs.",
                24, getHeight() - 28, getWidth() - 48, 18, juce::Justification::left);
}
