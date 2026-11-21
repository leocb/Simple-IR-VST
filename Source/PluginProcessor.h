#pragma once

#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>

/**
    A convolution plugin: loads an impulse response from an audio file on disk
    and blends the convolved (wet) signal with the dry input.

    The processor itself is kept intentionally small: one parameter (mix) plus
    an IR file path that is stored in the plugin state.
*/
class IRLoaderAudioProcessor final : public juce::AudioProcessor
{
public:
    IRLoaderAudioProcessor();
    ~IRLoaderAudioProcessor() override = default;

    static constexpr const char* mixParameterID = "mix";
    static constexpr const char* irPathProperty = "irPath";

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return irDurationSeconds; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    //==============================================================================
    /** Loads an impulse response file. Call this from the message thread.
        Returns false and fills errorMessage if the file cannot be used.
    */
    bool loadIRFromFile (const juce::File& file, juce::String& errorMessage);

    juce::File getIRFile() const { return irFile; }
    juce::String getIRName() const { return irFile.getFileName(); }
    double getIRDurationSeconds() const { return irDurationSeconds; }
    double getIRSampleRate() const { return irSampleRate; }
    int getIRNumChannels() const { return irNumChannels; }

    /** True once an IR has been requested. Until then the plugin is transparent. */
    bool hasIR() const { return hasLoadedIR.load (std::memory_order_relaxed); }

    juce::AudioProcessorValueTreeState apvts;

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    void unloadIR();

    juce::AudioFormatManager formatManager;
    juce::dsp::Convolution convolution;
    juce::dsp::DryWetMixer<float> dryWetMixer;

    std::atomic<bool> hasLoadedIR { false };
    juce::File irFile;
    double irDurationSeconds = 0.0;
    double irSampleRate = 0.0;
    int irNumChannels = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (IRLoaderAudioProcessor)
};
