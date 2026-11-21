#pragma once

#include "PluginProcessor.h"

#include <juce_gui_basics/juce_gui_basics.h>

class IRDropPanel;

class IRLoaderAudioProcessorEditor final : public juce::AudioProcessorEditor,
                                           private juce::Timer
{
public:
    explicit IRLoaderAudioProcessorEditor (IRLoaderAudioProcessor&);
    ~IRLoaderAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void chooseIRFile();
    void loadIRFile (const juce::File&);
    void updateIRDisplay();
    void updateMixValue();

    IRLoaderAudioProcessor& audioProcessor;

    std::unique_ptr<juce::LookAndFeel_V4> lookAndFeel;
    std::unique_ptr<IRDropPanel> irPanel;

    juce::Slider mixSlider;
    juce::Label mixCaption;
    juce::Label mixValue;
    juce::Label mixHint;

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> mixAttachment;
    std::unique_ptr<juce::FileChooser> fileChooser;

    juce::String displayedIRPath;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (IRLoaderAudioProcessorEditor)
};
