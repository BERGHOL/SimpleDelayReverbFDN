/*
  ==============================================================================
    PluginProcessor.h
    SimpleDelayReverbFDN – processeur audio (Delay / Reverb simple)
  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

//==============================================================================
// Classe processeur : gère le traitement audio (DSP)
//==============================================================================

class SimpleReverbAudioProcessor : public juce::AudioProcessor
{
public:
    //==========================================================================
    SimpleReverbAudioProcessor();
    ~SimpleReverbAudioProcessor() override;

    // Cycle de vie audio
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

#ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
#endif

    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==========================================================================
    // Interface / hôte
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==========================================================================
    // Programmes (peu utilisés, on laisse la version par défaut)
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override;
    void changeProgramName(int index, const juce::String& newName) override;

    //==========================================================================
    // Sauvegarde d’état
    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    //==========================================================================
    // Paramètres exposés à l’UI (APVTS)
    using APVTS = juce::AudioProcessorValueTreeState;
    static APVTS::ParameterLayout createParameterLayout();
    APVTS apvts{ *this, nullptr, "PARAMS", createParameterLayout() };

private:
    //==========================================================================
    // DSP interne

    // --- Delay ---
    double currentSampleRate = 44100.0;
    juce::AudioBuffer<float> delayBuffer;  // buffer circulaire pour le delay
    int delayWritePosition = 0;

    // --- Reverb JUCE ---
    juce::Reverb reverb;

    //==========================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SimpleReverbAudioProcessor)
};
