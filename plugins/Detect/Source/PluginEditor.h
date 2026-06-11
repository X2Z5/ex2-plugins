#pragma once
#include "PluginProcessor.h"

class EX2DetectEditor : public juce::AudioProcessorEditor,
                        private juce::Timer
{
public:
    explicit EX2DetectEditor (EX2DetectProcessor&);
    ~EX2DetectEditor() override = default;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override { repaint(); }

    EX2DetectProcessor& proc;
    juce::TextButton resetBtn { "RESET" };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EX2DetectEditor)
};
