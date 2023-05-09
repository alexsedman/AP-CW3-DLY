/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
APCW3DLYAudioProcessor::APCW3DLYAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::mono(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::mono(), true)
                     #endif
                       ),
                       // Instantiating the constructor layout for the params object.
                       params (*this, nullptr, juce::Identifier("PARAMETERS"), createParameters())
#endif
{
}

APCW3DLYAudioProcessor::~APCW3DLYAudioProcessor()
{
}

//==============================================================================
const juce::String APCW3DLYAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool APCW3DLYAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool APCW3DLYAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool APCW3DLYAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double APCW3DLYAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int APCW3DLYAudioProcessor::getNumPrograms()
{
    return 1;
}

int APCW3DLYAudioProcessor::getCurrentProgram()
{
    return 0;
}

void APCW3DLYAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String APCW3DLYAudioProcessor::getProgramName (int index)
{
    return {};
}

void APCW3DLYAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

void APCW3DLYAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // Initialise DSP delay line specs for use in the delay module.
    juce::dsp::ProcessSpec spec;
    spec.maximumBlockSize = samplesPerBlock;
    spec.sampleRate = sampleRate;
    delayModule.prepare(spec);
    
    tremPos = 0; // This variable measures how much tremolo to apply to each sample.
}

void APCW3DLYAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool APCW3DLYAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

void APCW3DLYAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels  = getTotalNumOutputChannels();
    
    // Load inital params.
    auto bypass = params.getRawParameterValue("BYPASS")->load();
    auto gain = params.getRawParameterValue("GAIN")->load();
    auto tremOn = params.getRawParameterValue("TREMOLO")->load();
    
    // Additional const var defined: The total number of channels
    const auto numChannels = juce::jmax (totalNumInputChannels, totalNumOutputChannels);
    
    for (int channel = 0; channel < totalNumInputChannels; ++channel)
    {
        // If bypass is on, then skip the delay module...
        if (bypass)
        {
            buffer.applyGain(gain);
            continue;
        }
        // Else activate the delay effects...
        else
        {
            processDelay(channel, numChannels, buffer, gain);
            
            if (tremOn)
                applyTremolo(channel, buffer);
        }
    }
}

//==============================================================================
// Delay processing function.
void APCW3DLYAudioProcessor::processDelay(int channel, int numChannels, juce::AudioBuffer<float>& buffer, float gain)
{
    // Declaring some parameter values at the start of the process block...
    auto delayTime = params.getRawParameterValue("TIME")->load();
    auto feedback = params.getRawParameterValue("FEEDBACK")->load();

    // Create a delay line audio buffer using the juce::DSP object.
    auto delayBuffer = juce::dsp::AudioBlock<float> (buffer).getSubsetChannelBlock(0, numChannels);
    
    // Set the delay line length...
    delayModule.setDelay(delayTime * (getSampleRate() / 1000));
    
    // Load wetness values...
    auto wetness = static_cast<float>(params.getRawParameterValue("WETNESS")->load()) / 100.0f;
    
    // Define the delay buffer context - finds information relating to the delay buffer, in preparation for running through the delay module:
    auto context = juce::dsp::ProcessContextReplacing<float>(delayBuffer);
    const auto& delayInput = context.getInputBlock(); // The delay buffer input sample.
    const auto& delayOutput = context.getOutputBlock(); // The delay buffer output sample.
    auto* delaySampleIn = delayInput.getChannelPointer (channel); // The delay buffer input sample position.
    auto* delaySampleOut = delayOutput.getChannelPointer (channel); // The delay buffer output sample position.
    
    // For each sample to be processed through the delay line...
    for (int sample = 0; sample < delayInput.getNumSamples(); ++sample)
    {
        // Gets the current input and output samples of the delay module...
        auto delayInput = delaySampleIn[sample];
        auto delayOutput = delayModule.popSample(channel);
        
        // Calculate a new input sample for the delay module, with some fedback output, and push back into the module...
        auto newDelayInput = delaySampleIn[sample] + feedback * delayOutput;
        delayModule.pushSample(channel, newDelayInput);
        
        // Creates a gain staged, dry/wet output from the input into the delay and the output of the delay.
        delaySampleOut[sample] = gain * (delayInput * (1.0f - wetness) + delayOutput * wetness);
    }
}

//==============================================================================
// Tremolo processing function.
void APCW3DLYAudioProcessor::applyTremolo(int channel, juce::AudioBuffer<float>& buffer)
{
    // Declare initial variables.
    auto tremFreq = params.getRawParameterValue("TREMSPEED")->load();
    float* channelData = buffer.getWritePointer (channel);
    const int period = (1 / tremFreq) * getSampleRate();
    
    // For each sample from the audio buffer, a sinusoidal tremolo effect is applied using basic sinusoidal theory.
    for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
    {
        channelData[sample] *= std::sin(twoPi * tremFreq * tremPos / getSampleRate()); // sin(2pi*f*(sample/rate))
        tremPos++; // Increment the tremolo position
        // If we've reached the end of the cycle...
        if (tremPos >= period)
            tremPos -= period; // Wrap the position around using the modulo operator.
    }
}

//==============================================================================
bool APCW3DLYAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* APCW3DLYAudioProcessor::createEditor()
{
    //return new APCW3DLYAudioProcessorEditor (*this); NOTE - FOR DEBUGGING
    // Here, the GenericAudioProcessorEditor provides a clean UI with all parameters laid out for use.
    return new juce::GenericAudioProcessorEditor (*this);
}

void APCW3DLYAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
}

void APCW3DLYAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new APCW3DLYAudioProcessor();
}

//==============================================================================
// Value tree state object. Returns a vector with all parameter elements.
juce::AudioProcessorValueTreeState::ParameterLayout APCW3DLYAudioProcessor::createParameters()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;
    
    // Delay parameters.
    params.push_back (std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("GAIN", 1), "Gain", 0.0f, 1.0f, 0.5f));
    params.push_back (std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("FEEDBACK", 1), "Feedback", 0.0f, 1.0f, 0.5f));
    params.push_back (std::make_unique<juce::AudioParameterInt>(juce::ParameterID("WETNESS", 1), "Wetness", 0, 100, 50, "%"));
    params.push_back (std::make_unique<juce::AudioParameterInt>(juce::ParameterID("TIME", 1), "Delay Time", 100, 400, 200, "ms"));
    params.push_back (std::make_unique<juce::AudioParameterBool>(juce::ParameterID("BYPASS", 1), "Bypass", 0));
    
    // Tremolo parameters.
    params.push_back (std::make_unique<juce::AudioParameterBool>(juce::ParameterID("TREMOLO", 1), "Tremolo", 0));
    params.push_back (std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("TREMSPEED", 1), "Tremolo Speed", 0.1f, 10.0f, 0.1f));
    
    return {params.begin(), params.end()};
}
