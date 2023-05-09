/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

//==============================================================================
/**
*/
class APCW3DLYAudioProcessor  : public juce::AudioProcessor
                            #if JucePlugin_Enable_ARA
                             , public juce::AudioProcessorARAExtension
                            #endif
{
public:
    //==============================================================================
    APCW3DLYAudioProcessor();
    ~APCW3DLYAudioProcessor() override;

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
    
private:
    void applyTremolo(int channel, juce::AudioBuffer<float>& buffer); // Tremolo function is declared here.
    void processDelay(int channel, int numChannels, juce::AudioBuffer<float>& buffer, float gain); // The delay function is declared here.
    
    static constexpr auto DelayLineLength = 192000; // Set a maximum delay line length
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Linear> delayModule {DelayLineLength}; // Delay line module using the juce::DSP object.
    
    // Define tremolo position, and 2*pi.
    float tremPos;
    const long double twoPi = 3.14159265358979323846264338328L;
    
    //==============================================================================
    // Declare the createParameter() function, which will set up all parameters.
    juce::AudioProcessorValueTreeState::ParameterLayout createParameters();
    // Naming the params object.
    juce::AudioProcessorValueTreeState params;

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (APCW3DLYAudioProcessor)
};
