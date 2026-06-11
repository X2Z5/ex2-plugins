#pragma once
#include "PluginProcessor.h"

//==============================================================================
class MidiDragSpot : public juce::Component
{
public:
    explicit MidiDragSpot (EX2ChordsProcessor& p) : proc (p)
    {
        setMouseCursor (juce::MouseCursor::DraggingHandCursor);
    }

    void paint (juce::Graphics& g) override
    {
        auto r = getLocalBounds().toFloat().reduced (2.0f);
        g.setColour (juce::Colour (0xff2a2440));
        g.fillRoundedRectangle (r, 10.0f);
        g.setColour (juce::Colour (0xff59e3ff).withAlpha (hover ? 1.0f : 0.55f));
        const float dash[] = { 6.0f, 4.0f };
        juce::Path outline, dashed;
        outline.addRoundedRectangle (r.reduced (2.0f), 8.0f);
        juce::PathStrokeType (1.6f).createDashedStroke (dashed, outline, dash, 2);
        g.fillPath (dashed);
        g.setColour (juce::Colours::white.withAlpha (hover ? 1.0f : 0.8f));
        g.setFont (juce::Font (juce::FontOptions (15.0f)));
        g.drawText ("DRAG MIDI INTO FL", getLocalBounds(), juce::Justification::centred);
    }

    void mouseEnter (const juce::MouseEvent&) override { hover = true;  repaint(); }
    void mouseExit  (const juce::MouseEvent&) override { hover = false; repaint(); }
    void mouseUp    (const juce::MouseEvent&) override { dragging = false; }

    void mouseDrag (const juce::MouseEvent& e) override
    {
        if (dragging || e.getDistanceFromDragStart() < 8)
            return;
        dragging = true;
        auto f = proc.writeMidiToTemp();
        if (f.existsAsFile())
            juce::DragAndDropContainer::performExternalDragDropOfFiles ({ f.getFullPathName() }, false, this, nullptr);
    }

private:
    EX2ChordsProcessor& proc;
    bool dragging = false, hover = false;
};

//==============================================================================
class EX2ChordsEditor : public juce::AudioProcessorEditor,
                        private juce::Timer
{
public:
    explicit EX2ChordsEditor (EX2ChordsProcessor&);
    ~EX2ChordsEditor() override = default;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void setupCombo (juce::ComboBox&, const char* paramID,
                     std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment>&);

    EX2ChordsProcessor& proc;

    juce::ComboBox keyBox, progBox, rhythmBox, extBox, bassBox, octBox;
    juce::Slider humanSlider { juce::Slider::RotaryHorizontalVerticalDrag, juce::Slider::NoTextBox };
    juce::TextButton diceBtn { "DICE" }, previewBtn { "PREVIEW" }, saveBtn { "SAVE .MID" };
    juce::ToggleButton hostMidiBtn { "MIDI out while host plays" };
    MidiDragSpot dragSpot;

    using ComboAtt  = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    using SliderAtt = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAtt = juce::AudioProcessorValueTreeState::ButtonAttachment;
    std::unique_ptr<ComboAtt> keyAtt, progAtt, rhythmAtt, extAtt, bassAtt, octAtt;
    std::unique_ptr<SliderAtt> humanAtt;
    std::unique_ptr<ButtonAtt> previewAtt, hostMidiAtt;

    std::vector<EX2ChordsProcessor::DisplayChord> shown;
    std::unique_ptr<juce::FileChooser> chooser;
    juce::Rectangle<int> stripArea;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EX2ChordsEditor)
};
