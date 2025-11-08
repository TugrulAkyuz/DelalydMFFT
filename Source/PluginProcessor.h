#pragma once

#include <JuceHeader.h>
#include <vector>
#include <complex>

static constexpr int fftOrder = 11;
static constexpr int fftSize = 1 << fftOrder;      // 1024 samples
static constexpr int numBins = fftSize / 2 + 1;    // 513 bins
static constexpr int overlap = 4;                  // 75% overlap
static constexpr int hopSize = fftSize / overlap;  // 256 samples
static constexpr int FFTBUFFER =  50 ;

//==============================================================================
/**
*/
class DelalydMFFTAudioProcessor  : public juce::AudioProcessor
                            #if JucePlugin_Enable_ARA
                             , public juce::AudioProcessorARAExtension
                            #endif
{
public:
    //==============================================================================
    DelalydMFFTAudioProcessor();
    ~DelalydMFFTAudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    //==============================================================================
    // Custom methods and parameters
    juce::AudioProcessorValueTreeState parameters;
    
    int getDelayFrames() const { return delayFrames; }
    void setDelayFrames(int newDelay);

private:
    //==============================================================================
    void processFFT(juce::AudioBuffer<float>& buffer);
    void updateFFTSize(int newSize);
    
    // FFT related members
     
    std::unique_ptr<juce::dsp::FFT> fft;
    std::unique_ptr<juce::dsp::FFT> ifft;
    std::vector<std::complex<float>> fftInput, fftOutput, ifftOutput;
    std::vector<std::vector<std::complex<float>>> fftCircularBuffer;
    int currentBufferIndex;

    // Hann window
    //std::vector<float> window;

    juce::AudioBuffer<float> intermediateBuffer;
    int intermediateBufferIndex;
    
    juce::AudioBuffer<float> outputBuffer;
    int outputBufferIndex = 0;
    int outputReadBufferIndex = 0;;
    int outputBufferSize;
    juce::dsp::WindowingFunction<float> window;
    juce::dsp::FFT forwardFFT;
    float fftInbuffer[2][fftSize] = {};
    float fftDelayInbuffer[2][FFTBUFFER][fftSize] = {};
    float fftDelayInbufferSnapShot[2][fftSize] = {};
    int fftDelayReadbuffer = 0;
    int fftDelayWritebuffer = 0;

    // Delay frames
    int delayFrames;
    int counter = 0;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DelalydMFFTAudioProcessor)
};
