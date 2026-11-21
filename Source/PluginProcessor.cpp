#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
IRLoaderAudioProcessor::IRLoaderAudioProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput ("Input", juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMETERS", createParameterLayout())
{
    formatManager.registerBasicFormats();
}

juce::AudioProcessorValueTreeState::ParameterLayout
IRLoaderAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { mixParameterID, 1 },
        "Mix",
        juce::NormalisableRange<float> (0.0f, 100.0f, 1.0f),
        100.0f,
        juce::AudioParameterFloatAttributes()
            .withLabel ("%")
            .withStringFromValueFunction ([] (float value, int)
            {
                return juce::String (juce::roundToInt (value)) + " %";
            })));

    return layout;
}

//==============================================================================
void IRLoaderAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    const auto numChannels = juce::jmax (1, getTotalNumOutputChannels());
    const juce::dsp::ProcessSpec spec { sampleRate,
                                        (juce::uint32) juce::jmax (1, samplesPerBlock),
                                        (juce::uint32) numChannels };

    convolution.prepare (spec);

    dryWetMixer.setMixingRule (juce::dsp::DryWetMixingRule::linear);
    dryWetMixer.setRampLength (juce::Seconds (0.02));
    dryWetMixer.setWetLatency ((float) convolution.getLatency());
    dryWetMixer.prepare (spec);
}

void IRLoaderAudioProcessor::releaseResources()
{
}

bool IRLoaderAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& input = layouts.getMainInputChannelSet();
    const auto& output = layouts.getMainOutputChannelSet();

    if (input != output)
        return false;

    return output == juce::AudioChannelSet::mono()
        || output == juce::AudioChannelSet::stereo();
}

void IRLoaderAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ignoreUnused (midiMessages);
    juce::ScopedNoDenormals noDenormals;

    const auto numSamples = buffer.getNumSamples();

    // Clear any output channels the host may have added that we don't use.
    for (int channel = getTotalNumInputChannels(); channel < getTotalNumOutputChannels(); ++channel)
        buffer.clear (channel, 0, numSamples);

    // No IR has been loaded yet: behave like a plain pass-through.
    if (numSamples == 0 || ! hasIR())
        return;

    const auto mix = juce::jlimit (0.0f, 1.0f,
                                   apvts.getRawParameterValue (mixParameterID)->load() * 0.01f);

    juce::dsp::AudioBlock<float> block (buffer);

    dryWetMixer.setWetMixProportion (mix);
    dryWetMixer.pushDrySamples (block);

    juce::dsp::ProcessContextReplacing<float> context (block);
    convolution.process (context);

    dryWetMixer.mixWetSamples (block);
}

//==============================================================================
bool IRLoaderAudioProcessor::loadIRFromFile (const juce::File& file, juce::String& errorMessage)
{
    if (! file.existsAsFile())
    {
        errorMessage = "The file does not exist: " + file.getFullPathName();
        return false;
    }

    // Validate the file up front so we never hand a broken file to the
    // convolution engine (which loads asynchronously in the background).
    std::unique_ptr<juce::AudioFormatReader> reader (formatManager.createReaderFor (file));

    if (reader == nullptr || reader->sampleRate <= 0.0 || reader->lengthInSamples <= 0)
    {
        errorMessage = "Not a supported audio file: " + file.getFileName();
        return false;
    }

    const auto duration = (double) reader->lengthInSamples / reader->sampleRate;
    const auto sourceSampleRate = reader->sampleRate;
    const auto sourceChannels = (int) reader->numChannels;
    reader.reset();

    convolution.loadImpulseResponse (file,
                                     juce::dsp::Convolution::Stereo::yes,
                                     juce::dsp::Convolution::Trim::yes,
                                     0,
                                     juce::dsp::Convolution::Normalise::yes);

    irFile = file;
    irDurationSeconds = duration;
    irSampleRate = sourceSampleRate;
    irNumChannels = sourceChannels;
    hasLoadedIR.store (true, std::memory_order_relaxed);

    apvts.state.setProperty (irPathProperty, file.getFullPathName(), nullptr);

    return true;
}

void IRLoaderAudioProcessor::unloadIR()
{
    irFile = juce::File();
    irDurationSeconds = 0.0;
    irSampleRate = 0.0;
    irNumChannels = 0;
    hasLoadedIR.store (false, std::memory_order_relaxed);
}

//==============================================================================
juce::AudioProcessorEditor* IRLoaderAudioProcessor::createEditor()
{
    return new IRLoaderAudioProcessorEditor (*this);
}

void IRLoaderAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void IRLoaderAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    const auto xml = getXmlFromBinary (data, sizeInBytes);

    if (xml == nullptr || ! xml->hasTagName (apvts.state.getType()))
        return;

    auto newState = juce::ValueTree::fromXml (*xml);
    apvts.replaceState (newState);

    const auto path = newState.getProperty (irPathProperty, {}).toString();

    if (path.isEmpty())
    {
        unloadIR();
        return;
    }

    juce::String error;

    if (! loadIRFromFile (juce::File (path), error))
    {
        DBG ("IR Loader: could not restore impulse response: " << error);
        unloadIR();
    }
}

//==============================================================================
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new IRLoaderAudioProcessor();
}
