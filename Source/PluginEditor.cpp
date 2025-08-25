#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
DelalydMFFTAudioProcessorEditor::DelalydMFFTAudioProcessorEditor (DelalydMFFTAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    // Set up delay slider
    delaySlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    delaySlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 80, 20);
    delaySlider.setRange(1, 60, 1);
    delaySlider.setValue(10);
    addAndMakeVisible(delaySlider);
    
    delayLabel.setText("Delay Frames", juce::dontSendNotification);
    delayLabel.attachToComponent(&delaySlider, false);
    delayLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(delayLabel);
    
    // Attach slider to parameter
    delayAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.parameters, "delayFrames", delaySlider);
    
    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.
    setSize (400, 300);
}

DelalydMFFTAudioProcessorEditor::~DelalydMFFTAudioProcessorEditor()
{
}

//==============================================================================
void DelalydMFFTAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));

    g.setColour (juce::Colours::white);
    g.setFont (20.0f);
    g.drawFittedText ("Delalyd MFFT", getLocalBounds().removeFromTop(50), juce::Justification::centred, 1);
    
    g.setFont (15.0f);
    g.drawFittedText ("FFT Delay Multiplier", getLocalBounds().removeFromTop(80).removeFromBottom(30),
                     juce::Justification::centred, 1);
}

void DelalydMFFTAudioProcessorEditor::resized()
{
    // Position the slider
    delaySlider.setBounds(getWidth() / 2 - 100, getHeight() / 2 - 50, 200, 100);
    delayLabel.setBounds(getWidth() / 2 - 100, getHeight() / 2 + 60, 200, 20);
}
