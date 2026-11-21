#include "PluginProcessor.h"

#include <cmath>
#include <iostream>

namespace
{
//==============================================================================
int failures = 0;

void check (bool condition, const juce::String& message, int line)
{
    if (condition)
        return;

    ++failures;
    std::cerr << "FAIL [line " << line << "]: " << message << std::endl;
}

void checkClose (double actual, double expected, double tolerance,
                 const juce::String& message, int line)
{
    if (std::abs (actual - expected) <= tolerance)
        return;

    ++failures;
    std::cerr << "FAIL [line " << line << "]: " << message
              << " (expected " << expected << ", got " << actual << ")" << std::endl;
}

#define CHECK(cond) check ((cond), #cond, __LINE__)
#define CHECK_CLOSE(actual, expected, tolerance) \
    checkClose ((actual), (expected), (tolerance), #actual " ~= " #expected, __LINE__)

//==============================================================================
constexpr double sampleRate = 48000.0;
constexpr int blockSize = 256;

/** The test IR: 16 samples of 1.0 followed by 16 samples of 0.5. */
juce::AudioBuffer<float> makeTestIR (int numChannels = 1)
{
    juce::AudioBuffer<float> ir (numChannels, 32);
    ir.clear();

    for (int channel = 0; channel < numChannels; ++channel)
        for (int i = 0; i < ir.getNumSamples(); ++i)
            ir.setSample (channel, i, i < 16 ? 1.0f : 0.5f);

    return ir;
}

/** Energy normalisation factor JUCE applies when Normalise::yes is used. */
double normalisationFactorFor (const juce::AudioBuffer<float>& ir)
{
    double maxSumSquared = 0.0;

    for (int channel = 0; channel < ir.getNumChannels(); ++channel)
    {
        double sumSquared = 0.0;

        for (int i = 0; i < ir.getNumSamples(); ++i)
            sumSquared += (double) ir.getSample (channel, i) * ir.getSample (channel, i);

        maxSumSquared = juce::jmax (maxSumSquared, sumSquared);
    }

    return 0.125 / std::sqrt (maxSumSquared);
}

juce::File writeWav (const juce::AudioBuffer<float>& buffer, double rate = sampleRate)
{
    const auto file = juce::File::getSpecialLocation (juce::File::tempDirectory)
                          .getNonexistentChildFile ("ir-loader-test", ".wav");

    juce::WavAudioFormat format;
    std::unique_ptr<juce::OutputStream> stream (file.createOutputStream());

    if (stream == nullptr)
        return {};

    auto writer = format.createWriterFor (stream,
                                          juce::AudioFormatWriterOptions{}
                                              .withSampleRate (rate)
                                              .withNumChannels (buffer.getNumChannels())
                                              .withBitsPerSample (32));

    if (writer == nullptr)
        return {};

    writer->writeFromAudioSampleBuffer (buffer, 0, buffer.getNumSamples());
    writer.reset();   // flushes and closes the file

    return file;
}

void prepare (IRLoaderAudioProcessor& processor)
{
    processor.setPlayConfigDetails (2, 2, sampleRate, blockSize);
    processor.prepareToPlay (sampleRate, blockSize);
}

void setMix (IRLoaderAudioProcessor& processor, float percent)
{
    auto* parameter = processor.apvts.getParameter (IRLoaderAudioProcessor::mixParameterID);
    jassert (parameter != nullptr);
    parameter->setValueNotifyingHost (parameter->convertTo0to1 (percent));
}

void processBlock (IRLoaderAudioProcessor& processor, juce::AudioBuffer<float>& buffer)
{
    juce::MidiBuffer midi;
    processor.processBlock (buffer, midi);
}

void processSilence (IRLoaderAudioProcessor& processor, int numBlocks)
{
    juce::AudioBuffer<float> silence (2, blockSize);
    silence.clear();

    for (int i = 0; i < numBlocks; ++i)
        processBlock (processor, silence);
}

/** Lets parameter smoothing (20 ms) and IR crossfades finish. */
void settle (IRLoaderAudioProcessor& processor)
{
    processSilence (processor, 32); // 32 * 256 samples ~= 170 ms
}

juce::AudioBuffer<float> processImpulse (IRLoaderAudioProcessor& processor)
{
    juce::AudioBuffer<float> buffer (2, blockSize);
    buffer.clear();
    buffer.setSample (0, 0, 1.0f);
    buffer.setSample (1, 0, 1.0f);

    processBlock (processor, buffer);
    return buffer;
}

//==============================================================================
void testTransparentWithoutIR()
{
    IRLoaderAudioProcessor processor;
    prepare (processor);

    juce::AudioBuffer<float> buffer (2, 64);
    buffer.clear();
    buffer.setSample (0, 0, 0.75f);
    buffer.setSample (1, 0, -0.25f);

    processBlock (processor, buffer);

    CHECK_CLOSE (buffer.getSample (0, 0), 0.75, 1.0e-6);
    CHECK_CLOSE (buffer.getSample (1, 0), -0.25, 1.0e-6);
    CHECK (! processor.hasIR());
}

void testWetOnly()
{
    IRLoaderAudioProcessor processor;
    prepare (processor);

    const auto ir = makeTestIR();
    const auto file = writeWav (ir);
    CHECK (file.existsAsFile());

    juce::String error;
    CHECK (processor.loadIRFromFile (file, error));
    CHECK (error.isEmpty());
    CHECK (processor.hasIR());
    CHECK (processor.getIRName() == file.getFileName());
    CHECK_CLOSE (processor.getIRDurationSeconds(), 32.0 / sampleRate, 1.0e-9);
    CHECK_CLOSE (processor.getIRSampleRate(), sampleRate, 1.0e-9);
    CHECK (processor.getIRNumChannels() == 1);

    // prepare() forces the pending IR to become active.
    processor.prepareToPlay (sampleRate, blockSize);
    setMix (processor, 100.0f);
    settle (processor);

    const auto output = processImpulse (processor);
    const auto factor = normalisationFactorFor (ir);

    for (int i = 0; i < 32; ++i)
    {
        const auto expected = factor * (i < 16 ? 1.0 : 0.5);

        // A mono IR is applied to both channels.
        CHECK_CLOSE (output.getSample (0, i), expected, 1.0e-5);
        CHECK_CLOSE (output.getSample (1, i), expected, 1.0e-5);
    }

    for (int i = 32; i < blockSize; ++i)
        CHECK_CLOSE (output.getSample (0, i), 0.0, 1.0e-7);

    file.deleteFile();
}

void testFullyDry()
{
    IRLoaderAudioProcessor processor;
    prepare (processor);

    const auto file = writeWav (makeTestIR());
    juce::String error;
    CHECK (processor.loadIRFromFile (file, error));
    processor.prepareToPlay (sampleRate, blockSize);

    setMix (processor, 0.0f);
    settle (processor);

    const auto output = processImpulse (processor);

    CHECK_CLOSE (output.getSample (0, 0), 1.0, 1.0e-5);
    CHECK_CLOSE (output.getSample (1, 0), 1.0, 1.0e-5);

    for (int i = 1; i < blockSize; ++i)
    {
        CHECK_CLOSE (output.getSample (0, i), 0.0, 1.0e-7);
        CHECK_CLOSE (output.getSample (1, i), 0.0, 1.0e-7);
    }

    file.deleteFile();
}

void testHalfMix()
{
    IRLoaderAudioProcessor processor;
    prepare (processor);

    const auto ir = makeTestIR();
    const auto file = writeWav (ir);
    juce::String error;
    CHECK (processor.loadIRFromFile (file, error));
    processor.prepareToPlay (sampleRate, blockSize);

    setMix (processor, 50.0f);
    settle (processor);

    const auto output = processImpulse (processor);
    const auto factor = normalisationFactorFor (ir);

    for (int i = 0; i < 32; ++i)
    {
        const auto wet = factor * (i < 16 ? 1.0 : 0.5);
        const auto dry = i == 0 ? 1.0 : 0.0;
        CHECK_CLOSE (output.getSample (0, i), 0.5 * dry + 0.5 * wet, 1.0e-5);
    }

    file.deleteFile();
}

void testHotSwapWhilePlaying()
{
    IRLoaderAudioProcessor processor;
    prepare (processor);

    juce::String error;
    CHECK (processor.loadIRFromFile (writeWav (makeTestIR()), error));
    processor.prepareToPlay (sampleRate, blockSize);
    setMix (processor, 100.0f);
    settle (processor);

    juce::AudioBuffer<float> irB (1, 8);
    irB.clear();
    for (int i = 0; i < irB.getNumSamples(); ++i)
        irB.setSample (0, i, 0.25f);

    const auto fileB = writeWav (irB);
    CHECK (processor.loadIRFromFile (fileB, error));

    const auto expected = 0.25 * 0.125 / std::sqrt (8.0 * 0.0625);
    bool becameActive = false;
    double lastValue = 0.0;

    // IR loading happens on a background thread; give it wall-clock time to
    // finish and the audio thread time to swap engines.
    for (int attempt = 0; attempt < 100 && ! becameActive; ++attempt)
    {
        const auto output = processImpulse (processor);
        lastValue = output.getSample (0, 0);

        if (std::abs (lastValue - expected) < 1.0e-3)
        {
            becameActive = true;
            CHECK_CLOSE (output.getSample (0, 1), expected, 1.0e-3);
        }

        processSilence (processor, 2);
        juce::Thread::sleep (10);
    }

    if (! becameActive)
        std::cerr << "hot swap never became active, last output = " << lastValue
                  << ", expected = " << expected << std::endl;

    CHECK (becameActive);

    fileB.deleteFile();
}

void testStateRestore()
{
    const auto file = writeWav (makeTestIR());
    juce::MemoryBlock state;

    {
        IRLoaderAudioProcessor processor;
        prepare (processor);

        juce::String error;
        CHECK (processor.loadIRFromFile (file, error));
        setMix (processor, 42.0f);

        processor.getStateInformation (state);
    }

    CHECK (state.getSize() > 0);

    IRLoaderAudioProcessor restored;
    restored.setStateInformation (state.getData(), (int) state.getSize());

    CHECK (restored.getIRFile() == file);
    CHECK_CLOSE (restored.apvts.getRawParameterValue (IRLoaderAudioProcessor::mixParameterID)->load(),
                 42.0, 1.0e-3);

    prepare (restored);
    settle (restored);

    const auto output = processImpulse (restored);
    const auto factor = normalisationFactorFor (makeTestIR());
    const auto expectedAtZero = 0.58 * 1.0 + 0.42 * factor;

    CHECK_CLOSE (output.getSample (0, 0), expectedAtZero, 1.0e-4);

    file.deleteFile();
}

void testInvalidFiles()
{
    IRLoaderAudioProcessor processor;
    prepare (processor);

    juce::String error;
    CHECK (! processor.loadIRFromFile (juce::File ("/definitely/not/here.wav"), error));
    CHECK (error.isNotEmpty());

    const auto textFile = juce::File::getSpecialLocation (juce::File::tempDirectory)
                              .getNonexistentChildFile ("ir-loader-not-audio", ".wav");
    textFile.replaceWithText ("this is not an audio file");

    error.clear();
    CHECK (! processor.loadIRFromFile (textFile, error));
    CHECK (error.isNotEmpty());
    CHECK (! processor.hasIR());

    textFile.deleteFile();
}

//==============================================================================
void renderEditorSnapshot (IRLoaderAudioProcessor& processor, const juce::File& outputFile)
{
    std::unique_ptr<juce::AudioProcessorEditor> editor (processor.createEditor());
    editor->setSize (480, 232);

    juce::Image image (juce::Image::ARGB, 480, 232, true);
    {
        juce::Graphics g (image);
        editor->paintEntireComponent (g, true);
    }

    outputFile.deleteFile();
    std::unique_ptr<juce::OutputStream> stream (outputFile.createOutputStream());
    juce::PNGImageFormat png;
    png.writeImageToStream (image, *stream);
}

void renderEditorPreviews (const juce::File& directory)
{
    directory.createDirectory();

    {
        IRLoaderAudioProcessor empty;
        prepare (empty);
        renderEditorSnapshot (empty, directory.getChildFile ("editor-empty.png"));
    }

    IRLoaderAudioProcessor loaded;
    prepare (loaded);

    // A stereo IR with a decaying tail, just for the preview.
    juce::AudioBuffer<float> ir (2, 2048);
    ir.clear();
    for (int channel = 0; channel < ir.getNumChannels(); ++channel)
        for (int i = 0; i < ir.getNumSamples(); ++i)
            ir.setSample (channel, i, std::exp (-(float) i / 180.0f) * std::sin ((float) i * 0.05f));

    const auto file = writeWav (ir);
    juce::String error;
    CHECK (loaded.loadIRFromFile (file, error));

    renderEditorSnapshot (loaded, directory.getChildFile ("editor-loaded.png"));
    file.deleteFile();
}
} // namespace

//==============================================================================
int main (int argc, char* argv[])
{
    juce::ScopedJuceInitialiser_GUI juceInitialiser;

    for (int i = 1; i < argc - 1; ++i)
    {
        if (juce::String (argv[i]) == "--render-ui")
        {
            renderEditorPreviews (juce::File (argv[i + 1]));
            return failures == 0 ? 0 : 1;
        }
    }

    testTransparentWithoutIR();
    testWetOnly();
    testFullyDry();
    testHalfMix();
    testHotSwapWhilePlaying();
    testStateRestore();
    testInvalidFiles();

    if (failures == 0)
    {
        std::cout << "All IR Loader tests passed." << std::endl;
        return 0;
    }

    std::cerr << failures << " check(s) failed." << std::endl;
    return 1;
}
