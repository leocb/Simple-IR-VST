#include "PluginEditor.h"

namespace
{
const juce::Colour backgroundTop    { 0xff191c1f };
const juce::Colour backgroundBottom { 0xff101214 };
const juce::Colour panelFill        { 0xff1b1f23 };
const juce::Colour panelOutline     { 0xff2b3136 };
const juce::Colour textColour       { 0xffe8e6e1 };
const juce::Colour dimText          { 0xff7f878d };
const juce::Colour accent           { 0xffe0a458 };
const juce::Colour knobBodyTop      { 0xff2a3035 };
const juce::Colour knobBodyBottom   { 0xff15181b };
const juce::Colour knobOutline      { 0xff3a4147 };
const juce::Colour trackColour      { 0xff2e343a };

juce::Font makeFont (float height, int style = juce::Font::plain, float kerning = 0.0f)
{
    return juce::Font (juce::FontOptions (height, style).withKerningFactor (kerning));
}

void drawWaveIcon (juce::Graphics& g, juce::Rectangle<float> area, juce::Colour colour)
{
    constexpr float heights[] = { 0.32f, 0.62f, 1.0f, 0.5f, 0.78f, 0.36f };
    constexpr int numBars = 6;
    constexpr float barWidth = 2.5f;
    constexpr float gap = 4.5f;
    const auto totalWidth = numBars * barWidth + (numBars - 1) * gap;

    g.setColour (colour);
    auto x = area.getCentreX() - totalWidth * 0.5f;

    for (auto h : heights)
    {
        const auto barHeight = area.getHeight() * h;
        g.fillRoundedRectangle (juce::Rectangle<float> (x, area.getCentreY() - barHeight * 0.5f,
                                                        barWidth, barHeight),
                                barWidth * 0.5f);
        x += barWidth + gap;
    }
}

juce::String describeIR (const IRLoaderAudioProcessor& processor)
{
    const auto rate = processor.getIRSampleRate();
    const auto channels = processor.getIRNumChannels();
    const auto seconds = processor.getIRDurationSeconds();

    const auto rateText = rate > 0.0 ? juce::String (rate / 1000.0, 1) + " kHz"
                                     : juce::String();

    const auto channelText = channels == 1 ? juce::String ("mono")
                                          : channels == 2 ? juce::String ("stereo")
                                                          : juce::String (channels) + " ch";

    const auto durationText = seconds >= 1.0 ? juce::String (seconds, 2) + " s"
                                             : juce::String (juce::roundToInt (seconds * 1000.0)) + " ms";

    if (rateText.isEmpty())
        return channelText + "   \u00b7   " + durationText;

    return rateText + "   \u00b7   " + channelText + "   \u00b7   " + durationText;
}
} // namespace

//==============================================================================
class IRLookAndFeel final : public juce::LookAndFeel_V4
{
public:
    void drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                           float sliderPos, float rotaryStartAngle, float rotaryEndAngle,
                           juce::Slider&) override
    {
        const auto bounds = juce::Rectangle<int> (x, y, width, height).toFloat().reduced (5.0f);
        const auto radius = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f;
        const auto centre = bounds.getCentre();
        const auto stroke = juce::jmax (3.0f, radius * 0.085f);
        const auto arcRadius = radius - stroke * 0.5f;
        const auto angle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);

        juce::Path track;
        track.addCentredArc (centre.x, centre.y, arcRadius, arcRadius, 0.0f,
                             rotaryStartAngle, rotaryEndAngle, true);
        g.setColour (trackColour);
        g.strokePath (track, juce::PathStrokeType (stroke, juce::PathStrokeType::curved,
                                                   juce::PathStrokeType::rounded));

        if (sliderPos > 0.0005f)
        {
            juce::Path value;
            value.addCentredArc (centre.x, centre.y, arcRadius, arcRadius, 0.0f,
                                 rotaryStartAngle, angle, true);
            g.setColour (accent);
            g.strokePath (value, juce::PathStrokeType (stroke, juce::PathStrokeType::curved,
                                                       juce::PathStrokeType::rounded));
        }

        const auto bodyRadius = arcRadius - stroke * 1.6f;

        juce::ColourGradient body (knobBodyTop, centre.x, centre.y - bodyRadius,
                                   knobBodyBottom, centre.x, centre.y + bodyRadius, false);
        g.setGradientFill (body);
        g.fillEllipse (centre.x - bodyRadius, centre.y - bodyRadius,
                       bodyRadius * 2.0f, bodyRadius * 2.0f);

        g.setColour (knobOutline);
        g.drawEllipse (centre.x - bodyRadius, centre.y - bodyRadius,
                       bodyRadius * 2.0f, bodyRadius * 2.0f, 1.0f);

        const auto pointerThickness = juce::jmax (2.0f, radius * 0.055f);
        const auto pointerLength = bodyRadius * 0.56f;

        juce::Path pointer;
        pointer.addRoundedRectangle (-pointerThickness * 0.5f,
                                     -bodyRadius * 0.82f,
                                     pointerThickness,
                                     pointerLength,
                                     pointerThickness * 0.5f);

        g.setColour (textColour);
        g.fillPath (pointer, juce::AffineTransform::rotation (angle).translated (centre));
    }
};

//==============================================================================
class IRDropPanel final : public juce::Component,
                          public juce::FileDragAndDropTarget
{
public:
    IRDropPanel()
    {
        setMouseCursor (juce::MouseCursor::PointingHandCursor);
    }

    std::function<void()> onClicked;
    std::function<void (const juce::File&)> onFileDropped;

    void setIRInfo (const juce::String& name, const juce::String& detail)
    {
        if (irName == name && irDetail == detail)
            return;

        irName = name;
        irDetail = detail;
        repaint();
    }

    void paint (juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat().reduced (0.5f);

        auto fill = panelFill;

        if (dragOver)
            fill = accent.withAlpha (0.10f);
        else if (hovered)
            fill = panelFill.brighter (0.06f);

        g.setColour (fill);
        g.fillRoundedRectangle (bounds, 10.0f);

        g.setColour (dragOver ? accent.withAlpha (0.95f)
                              : hovered ? accent.withAlpha (0.55f)
                                        : panelOutline);
        g.drawRoundedRectangle (bounds, 10.0f, 1.0f);

        drawWaveIcon (g, juce::Rectangle<float> (bounds.getCentreX() - 20.0f, 30.0f, 40.0f, 22.0f),
                      accent.withAlpha (dragOver ? 1.0f : 0.9f));

        const auto hasIR = irName.isNotEmpty();

        const auto title = dragOver ? juce::String ("Drop to load")
                                    : hasIR ? irName
                                            : juce::String ("Load an impulse response");

        const auto subtitle = dragOver ? juce::String ("Release the file to load it")
                                       : hasIR ? irDetail
                                               : juce::String ("Click here or drop a WAV, AIFF or FLAC file");

        g.setFont (makeFont (14.5f, juce::Font::bold));
        g.setColour (textColour);
        g.drawText (title,
                    juce::Rectangle<int> (12, 62, getWidth() - 24, 22),
                    juce::Justification::centred, true);

        g.setFont (makeFont (9.5f, juce::Font::plain, 0.04f));
        g.setColour (dimText);
        g.drawText (subtitle,
                    juce::Rectangle<int> (12, 85, getWidth() - 24, 18),
                    juce::Justification::centred, true);
    }

    void mouseEnter (const juce::MouseEvent&) override
    {
        hovered = true;
        repaint();
    }

    void mouseExit (const juce::MouseEvent&) override
    {
        hovered = false;
        repaint();
    }

    void mouseUp (const juce::MouseEvent& e) override
    {
        if (getLocalBounds().contains (e.getPosition()) && onClicked != nullptr)
            onClicked();
    }

    bool isInterestedInFileDrag (const juce::StringArray& files) override
    {
        for (const auto& path : files)
            if (juce::File (path).hasFileExtension ("wav;aif;aiff;flac"))
                return true;

        return false;
    }

    void fileDragEnter (const juce::StringArray&, int, int) override
    {
        setDragOver (true);
    }

    void fileDragExit (const juce::StringArray&) override
    {
        setDragOver (false);
    }

    void filesDropped (const juce::StringArray& files, int, int) override
    {
        setDragOver (false);

        for (const auto& path : files)
        {
            const auto file = juce::File (path);

            if (file.hasFileExtension ("wav;aif;aiff;flac"))
            {
                if (onFileDropped != nullptr)
                    onFileDropped (file);

                return;
            }
        }
    }

private:
    void setDragOver (bool shouldBeDraggingOver)
    {
        if (dragOver == shouldBeDraggingOver)
            return;

        dragOver = shouldBeDraggingOver;
        repaint();
    }

    juce::String irName, irDetail;
    bool hovered = false;
    bool dragOver = false;
};

//==============================================================================
IRLoaderAudioProcessorEditor::IRLoaderAudioProcessorEditor (IRLoaderAudioProcessor& p)
    : AudioProcessorEditor (&p),
      audioProcessor (p)
{
    lookAndFeel = std::make_unique<IRLookAndFeel>();
    setLookAndFeel (lookAndFeel.get());

    irPanel = std::make_unique<IRDropPanel>();
    irPanel->onClicked = [this] { chooseIRFile(); };
    irPanel->onFileDropped = [this] (const juce::File& file) { loadIRFile (file); };
    addAndMakeVisible (*irPanel);

    mixCaption.setText ("MIX", juce::dontSendNotification);
    mixCaption.setJustificationType (juce::Justification::centred);
    mixCaption.setFont (makeFont (9.0f, juce::Font::bold, 0.22f));
    mixCaption.setColour (juce::Label::textColourId, dimText);
    mixCaption.setInterceptsMouseClicks (false, false);
    addAndMakeVisible (mixCaption);

    mixSlider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    mixSlider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    mixSlider.setRotaryParameters (juce::MathConstants<float>::pi * 1.25f,
                                   juce::MathConstants<float>::pi * 2.75f,
                                   true);
    mixSlider.setDoubleClickReturnValue (true, 100.0);
    mixSlider.setName ("Mix");
    mixSlider.onValueChange = [this] { updateMixValue(); };
    addAndMakeVisible (mixSlider);

    mixValue.setJustificationType (juce::Justification::centred);
    mixValue.setFont (makeFont (13.0f, juce::Font::bold));
    mixValue.setColour (juce::Label::textColourId, textColour);
    mixValue.setInterceptsMouseClicks (false, false);
    addAndMakeVisible (mixValue);

    mixHint.setText ("DRY      WET", juce::dontSendNotification);
    mixHint.setJustificationType (juce::Justification::centred);
    mixHint.setFont (makeFont (8.0f, juce::Font::plain, 0.18f));
    mixHint.setColour (juce::Label::textColourId, dimText.withAlpha (0.8f));
    mixHint.setInterceptsMouseClicks (false, false);
    addAndMakeVisible (mixHint);

    mixAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        audioProcessor.apvts, IRLoaderAudioProcessor::mixParameterID, mixSlider);

    updateMixValue();
    updateIRDisplay();

    setSize (480, 232);
    setResizable (false, false);

    startTimerHz (4);
}

IRLoaderAudioProcessorEditor::~IRLoaderAudioProcessorEditor()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

void IRLoaderAudioProcessorEditor::paint (juce::Graphics& g)
{
    juce::ColourGradient background (backgroundTop, 0.0f, 0.0f,
                                     backgroundBottom, 0.0f, (float) getHeight(), false);
    g.setGradientFill (background);
    g.fillAll();

    const auto header = juce::Rectangle<int> (30, 20, getWidth() - 60, 22);

    g.setFont (makeFont (15.0f, juce::Font::bold, 0.24f));
    g.setColour (textColour);
    g.drawText ("IR LOADER", header, juce::Justification::centredLeft, false);

    g.setFont (makeFont (9.0f, juce::Font::bold, 0.2f));
    g.setColour (dimText.withAlpha (0.85f));
    g.drawText ("IMPULSE RESPONSE", header, juce::Justification::centredRight, false);

    g.setColour (juce::Colour (0xff24282c));
    g.fillRect (juce::Rectangle<int> (30, 52, getWidth() - 60, 1));
}

void IRLoaderAudioProcessorEditor::resized()
{
    irPanel->setBounds (30, 66, 292, 136);

    const auto knobX = 354;

    mixCaption.setBounds (knobX, 54, 96, 16);
    mixSlider.setBounds (knobX, 72, 96, 100);
    mixValue.setBounds (knobX, 172, 96, 18);
    mixHint.setBounds (knobX, 190, 96, 12);
}

void IRLoaderAudioProcessorEditor::timerCallback()
{
    updateIRDisplay();
}

void IRLoaderAudioProcessorEditor::updateIRDisplay()
{
    const auto file = audioProcessor.getIRFile();
    const auto path = file.getFullPathName();

    if (path == displayedIRPath)
        return;

    displayedIRPath = path;

    if (path.isEmpty())
    {
        irPanel->setIRInfo ({}, {});
        return;
    }

    irPanel->setIRInfo (file.getFileName(), describeIR (audioProcessor));
}

void IRLoaderAudioProcessorEditor::updateMixValue()
{
    mixValue.setText (juce::String (juce::roundToInt (mixSlider.getValue())) + " %",
                      juce::dontSendNotification);
}

void IRLoaderAudioProcessorEditor::chooseIRFile()
{
    auto start = audioProcessor.getIRFile();

    if (! start.existsAsFile())
        start = juce::File::getSpecialLocation (juce::File::userMusicDirectory);

    const auto startFolder = start.isDirectory() ? start : start.getParentDirectory();

    fileChooser = std::make_unique<juce::FileChooser> ("Load impulse response",
                                                       startFolder,
                                                       "*.wav;*.aif;*.aiff;*.flac");

    fileChooser->launchAsync (juce::FileBrowserComponent::openMode
                                  | juce::FileBrowserComponent::canSelectFiles,
                              [this] (const juce::FileChooser& chooser)
                              {
                                  const auto result = chooser.getResult();

                                  if (result.existsAsFile())
                                      loadIRFile (result);
                              });
}

void IRLoaderAudioProcessorEditor::loadIRFile (const juce::File& file)
{
    juce::String error;

    if (! audioProcessor.loadIRFromFile (file, error))
    {
        juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::WarningIcon,
                                                "Could not load impulse response",
                                                error,
                                                "OK",
                                                this);
        return;
    }

    updateIRDisplay();
}
