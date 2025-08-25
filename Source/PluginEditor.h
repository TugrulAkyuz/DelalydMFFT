#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

//==============================================================================
/**
*/
class DelalydMFFTAudioProcessorEditor  : public juce::AudioProcessorEditor
{
public:
    DelalydMFFTAudioProcessorEditor (DelalydMFFTAudioProcessor&);
    ~DelalydMFFTAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    DelalydMFFTAudioProcessor& audioProcessor;
    
    // GUI components
    juce::Slider delaySlider;
    juce::Label delayLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> delayAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DelalydMFFTAudioProcessorEditor)
};
