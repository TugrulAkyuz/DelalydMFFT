#include "PluginProcessor.h"
#include "PluginEditor.h"

static constexpr int fftOrder = 11;
static constexpr int fftSize = 1 << fftOrder;      // 1024 samples
static constexpr int numBins = fftSize / 2 + 1;    // 513 bins
static constexpr int overlap = 4;                  // 75% overlap
static constexpr int hopSize = fftSize / overlap;  // 256 samples


//==============================================================================
DelalydMFFTAudioProcessor::DelalydMFFTAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       ),
       parameters(*this, nullptr, "Parameters", {
           std::make_unique<juce::AudioParameterInt>(juce::ParameterID{"delayFrames",1}, "Delay Frames", 1, 1000, 10)
       }),forwardFFT(11),window (fftSize, juce::dsp::WindowingFunction<float>::hann,false)
#endif
{
    
     
    currentBufferIndex = 0;
    delayFrames = 10;
    intermediateBufferIndex = 0;
    outputBufferIndex = 0;
    outputBufferSize = fftSize;
    
    delayFrames = parameters.getRawParameterValue("delayFrames")->load();
}

DelalydMFFTAudioProcessor::~DelalydMFFTAudioProcessor()
{
}

//==============================================================================
void DelalydMFFTAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    fft = std::make_unique<juce::dsp::FFT>(std::log2(fftSize));
     ifft = std::make_unique<juce::dsp::FFT>(std::log2(fftSize));
     
     // Initialize buffers
     fftInput.resize(fftSize);
     fftOutput.resize(fftSize);
     ifftOutput.resize(fftSize);
     
     // Initialize circular buffer
     fftCircularBuffer.resize(60, std::vector<std::complex<float>>(fftSize, {0.0f, 0.0f}));
     currentBufferIndex = 0;
     
     // Create Hann window
    // window.resize(fftSize);
    /*
     for (int i = 0; i < fftSize; ++i) {
         window[i] = 0.5f * (1.0f - std::cos(2.0f * juce::MathConstants<float>::pi * i / (fftSize - 1)));
     }
     */
     
     // Initialize intermediate and output buffers
     intermediateBuffer.setSize(1, fftSize * 2); // Extra space for safety
     intermediateBuffer.clear();
     intermediateBufferIndex = 0;
     
     outputBuffer.setSize(2, fftSize * 2);
     outputBuffer.clear();
     outputBufferIndex = 0;
     outputBufferSize = fftSize;
    
     
}

void DelalydMFFTAudioProcessor::releaseResources()
{
    fft.reset();
    ifft.reset();
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool DelalydMFFTAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

void DelalydMFFTAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();
    
    // Clear unused output channels
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear(i, 0, buffer.getNumSamples());

    /*
    for(int i  = 0 ; i < buffer.getNumSamples();i++)
        buffer.setSample(0, i, fmod((tmpDebugIndex = tmpDebugIndex + 1)*0.001,1.0));
    
*/
    // Process input samples
    if (totalNumInputChannels > 0)
    {
        auto* inputData = buffer.getReadPointer(0);
    
        
        for (int i = 0; i < buffer.getNumSamples(); ++i)
        {
            // Fill intermediate buffer with input
            intermediateBuffer.setSample(0, intermediateBufferIndex, inputData[i]);
   
            
            intermediateBufferIndex++;
            
            // When intermediate buffer is full, process FFT
            if (intermediateBufferIndex >= fftSize)
            {
                processFFT(intermediateBuffer);
                intermediateBufferIndex = fftSize - hopSize; // Overlap için
                
                // Shift remaining samples for overlap
                for (int j = 0; j < fftSize - hopSize; j++)
                {
                    intermediateBuffer.setSample(0, j,
                        intermediateBuffer.getSample(0, j + hopSize));
                }
            }
            
            buffer.setSample(0, i, outputBuffer.getSample(0, outputReadBufferIndex)/4);
            buffer.setSample(1, i, outputBuffer.getSample(0, outputReadBufferIndex)/4);
            tmpfftInbuffer[outputReadBufferIndex] =  outputBuffer.getSample(0, outputReadBufferIndex)/4;
            outputBuffer.setSample(0, outputReadBufferIndex, 0);
           
            outputReadBufferIndex = (outputReadBufferIndex + 1)% (2 * fftSize);
   
            
        }

        
    }
    
}

void DelalydMFFTAudioProcessor::processFFT(juce::AudioBuffer<float>& inputBuffer)
{
    // Copy input to FFT buffer
    for (int i = 0; i < fftSize; ++i)
    {
        fftInbuffer[0][i] = inputBuffer.getSample(0, i);
    }

    // Apply window

    window.multiplyWithWindowingTable(fftInbuffer[0], fftSize);
    
    // Forward FFT
    forwardFFT.performRealOnlyForwardTransform(fftInbuffer[0], true);
    
    // Inverse FFT
    forwardFFT.performRealOnlyInverseTransform(fftInbuffer[0]);
    
    // Apply window again and scale
    window.multiplyWithWindowingTable(fftInbuffer[0], fftSize);

    const float scaleFactor = 1.0f;
    
    // Overlap-add to output buffer (NO window after inverse FFT)
    int tmp = outputBufferIndex;
    for (int i = 0; i < fftSize; ++i)
    {
        
       
        float currentSample = outputBuffer.getSample(0, tmp);
        float newSample = fftInbuffer[0][i] * scaleFactor;
        outputBuffer.setSample(0, tmp, currentSample + newSample);
        tmp = (tmp + 1)% (2 * fftSize);
    }
    outputBufferIndex = (outputBufferIndex + hopSize)%(2 * fftSize);
    outputBufferSize = fftSize;
    //outputBufferIndex = 0;
}



void DelalydMFFTAudioProcessor::setDelayFrames(int newDelay)
{
    delayFrames = juce::jlimit(1, 60, newDelay);
}

//==============================================================================
const juce::String DelalydMFFTAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool DelalydMFFTAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool DelalydMFFTAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool DelalydMFFTAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double DelalydMFFTAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int DelalydMFFTAudioProcessor::getNumPrograms()
{
    return 1;
}

int DelalydMFFTAudioProcessor::getCurrentProgram()
{
    return 0;
}

void DelalydMFFTAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String DelalydMFFTAudioProcessor::getProgramName (int index)
{
    return {};
}

void DelalydMFFTAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
bool DelalydMFFTAudioProcessor::hasEditor() const
{
    return true;
}

juce::AudioProcessorEditor* DelalydMFFTAudioProcessor::createEditor()
{
    return new DelalydMFFTAudioProcessorEditor (*this);
}

//==============================================================================
void DelalydMFFTAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // Save parameters
    juce::MemoryOutputStream stream(destData, true);
    parameters.state.writeToStream(stream);
}

void DelalydMFFTAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // Load parameters
    juce::ValueTree tree = juce::ValueTree::readFromData(data, sizeInBytes);
    if (tree.isValid()) {
        parameters.state = tree;
        delayFrames = parameters.getRawParameterValue("delayFrames")->load();
    }
}

//==============================================================================
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new DelalydMFFTAudioProcessor();
}
