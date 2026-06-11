#include "PluginEditor.h"

static const juce::Colour kBg1   (0xff141221);
static const juce::Colour kBg2   (0xff241636);
static const juce::Colour kPink  (0xffff5fa2);
static const juce::Colour kCyan  (0xff59e3ff);
static const juce::Colour kPanel (0xff211c33);

void EX2ChordsEditor::setupCombo (juce::ComboBox& box, const char* paramID,
                                  std::unique_ptr<ComboAtt>& att)
{
    if (auto* p = dynamic_cast<juce::AudioParameterChoice*> (proc.apvts.getParameter (paramID)))
        box.addItemList (p->choices, 1);

    box.setColour (juce::ComboBox::backgroundColourId, kPanel);
    box.setColour (juce::ComboBox::textColourId, juce::Colours::white);
    box.setColour (juce::ComboBox::outlineColourId, kCyan.withAlpha (0.35f));
    box.setColour (juce::ComboBox::arrowColourId, kPink);
    addAndMakeVisible (box);
    att = std::make_unique<ComboAtt> (proc.apvts, paramID, box);
}

EX2ChordsEditor::EX2ChordsEditor (EX2ChordsProcessor& p)
    : juce::AudioProcessorEditor (p), proc (p), dragSpot (p)
{
    setupCombo (keyBox,    "key",    keyAtt);
    setupCombo (progBox,   "prog",   progAtt);
    setupCombo (rhythmBox, "rhythm", rhythmAtt);
    setupCombo (extBox,    "ext",    extAtt);
    setupCombo (bassBox,   "bass",   bassAtt);
    setupCombo (octBox,    "octave", octAtt);

    humanSlider.setColour (juce::Slider::rotarySliderFillColourId, kPink);
    humanSlider.setColour (juce::Slider::rotarySliderOutlineColourId, kPanel.brighter (0.3f));
    humanSlider.setColour (juce::Slider::thumbColourId, kCyan);
    addAndMakeVisible (humanSlider);
    humanAtt = std::make_unique<SliderAtt> (proc.apvts, "humanize", humanSlider);

    diceBtn.setColour (juce::TextButton::buttonColourId, kPink.withAlpha (0.85f));
    diceBtn.setColour (juce::TextButton::textColourOffId, juce::Colours::white);
    diceBtn.onClick = [this] { proc.reroll(); };
    addAndMakeVisible (diceBtn);

    previewBtn.setClickingTogglesState (true);
    previewBtn.setColour (juce::TextButton::buttonColourId, kPanel);
    previewBtn.setColour (juce::TextButton::buttonOnColourId, kCyan.withAlpha (0.7f));
    previewBtn.setColour (juce::TextButton::textColourOffId, juce::Colours::white);
    previewBtn.setColour (juce::TextButton::textColourOnId, juce::Colours::black);
    addAndMakeVisible (previewBtn);
    previewAtt = std::make_unique<ButtonAtt> (proc.apvts, "preview", previewBtn);

    saveBtn.setColour (juce::TextButton::buttonColourId, kPanel);
    saveBtn.setColour (juce::TextButton::textColourOffId, juce::Colours::white);
    saveBtn.onClick = [this]
    {
        chooser = std::make_unique<juce::FileChooser> ("Save MIDI",
                      juce::File::getSpecialLocation (juce::File::userDesktopDirectory)
                          .getChildFile ("EX2 Chords.mid"),
                      "*.mid");
        chooser->launchAsync (juce::FileBrowserComponent::saveMode
                              | juce::FileBrowserComponent::warnAboutOverwriting,
            [this] (const juce::FileChooser& fc)
            {
                auto f = fc.getResult();
                if (f != juce::File())
                    proc.writeMidiTo (f.withFileExtension ("mid"));
            });
    };
    addAndMakeVisible (saveBtn);

    hostMidiBtn.setColour (juce::ToggleButton::textColourId, juce::Colours::white.withAlpha (0.75f));
    hostMidiBtn.setColour (juce::ToggleButton::tickColourId, kCyan);
    hostMidiBtn.setColour (juce::ToggleButton::tickDisabledColourId, juce::Colours::grey);
    addAndMakeVisible (hostMidiBtn);
    hostMidiAtt = std::make_unique<ButtonAtt> (proc.apvts, "hostmidi", hostMidiBtn);

    addAndMakeVisible (dragSpot);

    setSize (780, 470);
    startTimerHz (4);
}

void EX2ChordsEditor::timerCallback()
{
    auto latest = proc.getDisplayChords();
    bool changed = latest.size() != shown.size();
    if (! changed)
        for (size_t i = 0; i < latest.size(); ++i)
            if (latest[i].name != shown[i].name) { changed = true; break; }
    if (changed)
    {
        shown = std::move (latest);
        repaint();
    }
}

void EX2ChordsEditor::paint (juce::Graphics& g)
{
    g.setGradientFill (juce::ColourGradient (kBg1, 0, 0, kBg2, (float) getWidth(), (float) getHeight(), false));
    g.fillAll();

    g.setColour (kPink);
    g.setFont (juce::Font (juce::FontOptions (juce::Font::getDefaultSansSerifFontName(), 26.0f, juce::Font::bold)));
    g.drawText ("EX2 CHORDS", 24, 14, 300, 30, juce::Justification::left);
    g.setColour (juce::Colours::white.withAlpha (0.55f));
    g.setFont (juce::Font (juce::FontOptions (14.0f)));
    g.drawText ("J-pop progressions + bounce, straight into FL", 24, 42, 420, 20, juce::Justification::left);

    auto label = [&g] (const char* text, juce::Rectangle<int> above)
    {
        g.setColour (juce::Colours::white.withAlpha (0.6f));
        g.setFont (juce::Font (juce::FontOptions (12.0f)));
        g.drawText (text, above.getX(), above.getY() - 18, above.getWidth(), 16, juce::Justification::left);
    };
    label ("KEY",         keyBox.getBounds());
    label ("PROGRESSION", progBox.getBounds());
    label ("BOUNCE / RHYTHM", rhythmBox.getBounds());
    label ("CHORD FLAVOR", extBox.getBounds());
    label ("BASS",        bassBox.getBounds());
    label ("OCTAVE",      octBox.getBounds());
    label ("HUMANIZE",    humanSlider.getBounds().withTrimmedLeft (-6));

    // chord strip
    if (shown.empty())
        shown = proc.getDisplayChords();

    auto strip = stripArea.toFloat();
    g.setColour (juce::Colours::white.withAlpha (0.6f));
    g.setFont (juce::Font (juce::FontOptions (12.0f)));
    g.drawText ("YOUR PROGRESSION (4 BARS, LOOPS)", stripArea.getX(), stripArea.getY() - 18,
                stripArea.getWidth(), 16, juce::Justification::left);

    double totalBars = 0;
    for (auto& c : shown) totalBars += c.lengthBars;
    if (totalBars <= 0) totalBars = 4;

    float x = strip.getX();
    int i = 0;
    for (auto& c : shown)
    {
        const float w = (float) (c.lengthBars / totalBars) * strip.getWidth();
        auto box = juce::Rectangle<float> (x, strip.getY(), w - 6.0f, strip.getHeight());
        const juce::Colour col = juce::Colour::fromHSV ((0.83f + 0.09f * (float) i), 0.45f, 0.95f, 1.0f);
        g.setColour (col.withAlpha (0.16f));
        g.fillRoundedRectangle (box, 9.0f);
        g.setColour (col.withAlpha (0.85f));
        g.drawRoundedRectangle (box, 9.0f, 1.4f);
        g.setColour (juce::Colours::white);
        g.setFont (juce::Font (juce::FontOptions (juce::Font::getDefaultSansSerifFontName(),
                                                  w > 90 ? 20.0f : 15.0f, juce::Font::bold)));
        g.drawText (c.name, box.toNearestInt(), juce::Justification::centred);
        x += w;
        ++i;
    }
}

void EX2ChordsEditor::resized()
{
    const int colW = 226, h = 28, gap = 26;
    int y = 92;
    keyBox.setBounds    (24,              y, 110, h);
    progBox.setBounds   (24 + 130,        y, 340, h);
    rhythmBox.setBounds (24 + 130 + 360,  y, 240, h);
    y += h + gap + 14;
    extBox.setBounds    (24,              y, 170, h);
    bassBox.setBounds   (24 + 190,        y, 170, h);
    octBox.setBounds    (24 + 380,        y, 90,  h);
    humanSlider.setBounds (24 + 500,      y - 14, 54, 54);

    stripArea = juce::Rectangle<int> (24, y + h + gap + 14, getWidth() - 48, 86);

    const int by = stripArea.getBottom() + 22;
    diceBtn.setBounds    (24,  by, 110, 40);
    previewBtn.setBounds (148, by, 110, 40);
    dragSpot.setBounds   (272, by - 4, 280, 48);
    saveBtn.setBounds    (566, by, 110, 40);
    hostMidiBtn.setBounds (24, by + 48, 320, 24);
}
