/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
APCW3DLYAudioProcessorEditor::APCW3DLYAudioProcessorEditor (APCW3DLYAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    // Due to the use of the GenericAudioProcessorEditor, no UI elements are required here.
    setSize (600, 600);
}

APCW3DLYAudioProcessorEditor::~APCW3DLYAudioProcessorEditor()
{
}

//==============================================================================
void APCW3DLYAudioProcessorEditor::paint (juce::Graphics& g)
{
}

void APCW3DLYAudioProcessorEditor::resized()
{
}
