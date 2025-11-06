/*
  ==============================================================================
    PluginProcessor.cpp
    SimpleDelayReverbFDN – Delay / Reverb simple
  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
// Création de la liste de paramètres (APVTS)
//==============================================================================

SimpleReverbAudioProcessor::APVTS::ParameterLayout
SimpleReverbAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    // 0 = Delay, 1 = Reverb
    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        "mode", "Mode", juce::StringArray{ "Delay", "Reverb" }, 0));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "delayTimeMs", "Delay Time (ms)",
        juce::NormalisableRange<float>(1.0f, 1000.0f, 0.01f, 0.5f), 350.0f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "feedback", "Feedback",
        juce::NormalisableRange<float>(0.0f, 0.95f, 0.0f, 0.5f), 0.4f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "wet", "Wet",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.0f, 1.0f), 0.35f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "roomSize", "Room Size",
        juce::NormalisableRange<float>(0.1f, 1.0f, 0.0f, 0.7f), 0.6f));

    return { params.begin(), params.end() };
}

//==============================================================================
// Constructeur / destructeur
//==============================================================================

SimpleReverbAudioProcessor::SimpleReverbAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
    : AudioProcessor(BusesProperties()
#if !JucePlugin_IsMidiEffect
#if !JucePlugin_IsSynth
        .withInput("Input", juce::AudioChannelSet::stereo(), true)
#endif
        .withOutput("Output", juce::AudioChannelSet::stereo(), true)
#endif
    )
#endif
{
}

SimpleReverbAudioProcessor::~SimpleReverbAudioProcessor() = default;

//==============================================================================
// Infos générales
//==============================================================================

const juce::String SimpleReverbAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool SimpleReverbAudioProcessor::acceptsMidi() const
{
#if JucePlugin_WantsMidiInput
    return true;
#else
    return false;
#endif
}

bool SimpleReverbAudioProcessor::producesMidi() const
{
#if JucePlugin_ProducesMidiOutput
    return true;
#else
    return false;
#endif
}

bool SimpleReverbAudioProcessor::isMidiEffect() const
{
#if JucePlugin_IsMidiEffect
    return true;
#else
    return false;
#endif
}

double SimpleReverbAudioProcessor::getTailLengthSeconds() const
{
    // la reverb/delay peut durer > 0, mais on laisse 0 pour l’instant
    return 0.0;
}

//==============================================================================
// Programmes (non utilisés)
//==============================================================================

int SimpleReverbAudioProcessor::getNumPrograms() { return 1; }
int SimpleReverbAudioProcessor::getCurrentProgram() { return 0; }
void SimpleReverbAudioProcessor::setCurrentProgram(int) {}
const juce::String SimpleReverbAudioProcessor::getProgramName(int) { return {}; }
void SimpleReverbAudioProcessor::changeProgramName(int, const juce::String&) {}

//==============================================================================
// Préparation / libération
//==============================================================================

void SimpleReverbAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;

    // --- Buffer de delay ---
    const int maxDelayMs = 2000; // 2 s max
    int delayBufferSize = static_cast<int>(sampleRate * maxDelayMs / 1000.0) + samplesPerBlock;

    delayBuffer.setSize(getTotalNumOutputChannels(), delayBufferSize);
    delayBuffer.clear();
    delayWritePosition = 0;

    // --- Paramètres init de la reverb ---
    juce::Reverb::Parameters params;
    params.roomSize = 0.6f;
    params.damping = 0.5f;
    params.width = 1.0f;
    params.wetLevel = 0.3f;
    params.dryLevel = 0.7f;
    params.freezeMode = 0.0f;
    reverb.setParameters(params);
}

void SimpleReverbAudioProcessor::releaseResources()
{
    // Rien de spécial à libérer ici
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool SimpleReverbAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
#if JucePlugin_IsMidiEffect
    juce::ignoreUnused(layouts);
    return true;
#else
    // mono ou stéréo uniquement
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
        && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

#if !JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
#endif

    return true;
#endif
}
#endif

//==============================================================================
// Traitement audio : c’est ici que la magie Delay/Reverb se fait !
//==============================================================================

void SimpleReverbAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer,
    juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    juce::ignoreUnused(midiMessages);

    const int totalNumInputChannels = getTotalNumInputChannels();
    const int totalNumOutputChannels = getTotalNumOutputChannels();
    const int numSamples = buffer.getNumSamples();

    // Si on a plus de sorties que d’entrées, on nettoie les canaux inutilisés
    for (int ch = totalNumInputChannels; ch < totalNumOutputChannels; ++ch)
        buffer.clear(ch, 0, numSamples);

    // Récupération des paramètres
    auto* modeParam = apvts.getRawParameterValue("mode");
    auto* delayParam = apvts.getRawParameterValue("delayTimeMs");
    auto* fbParam = apvts.getRawParameterValue("feedback");
    auto* wetParam = apvts.getRawParameterValue("wet");
    auto* roomParam = apvts.getRawParameterValue("roomSize");

    const int   mode = static_cast<int>(*modeParam); // 0 = Delay, 1 = Reverb
    const float delayMs = *delayParam;
    const float feedback = *fbParam;
    const float wet = *wetParam;
    const float roomSize = *roomParam;

    const float dry = 1.0f - wet;

    // ----------------------------
    // MODE 0 : DELAY
    // ----------------------------
    if (mode == 0)
    {
        const int delayBufferSize = delayBuffer.getNumSamples();
        if (delayBufferSize == 0)
            return;

        const int delayInSamples = juce::jlimit(1,
            delayBufferSize - 1,
            (int)(currentSampleRate * delayMs / 1000.0f));

        for (int ch = 0; ch < totalNumInputChannels; ++ch)
        {
            float* channelData = buffer.getWritePointer(ch);
            float* delayData = delayBuffer.getWritePointer(ch);

            int writePos = delayWritePosition;

            for (int i = 0; i < numSamples; ++i)
            {
                const int readPos = (writePos - delayInSamples + delayBufferSize) % delayBufferSize;

                const float inSample = channelData[i];
                const float delayed = delayData[readPos];

                // feedback
                delayData[writePos] = inSample + delayed * feedback;

                // mix dry / wet
                channelData[i] = inSample * dry + delayed * wet;

                if (++writePos >= delayBufferSize)
                    writePos = 0;
            }
        }

        delayWritePosition += numSamples;
        delayWritePosition %= delayBuffer.getNumSamples();
    }
    // ----------------------------
    // MODE 1 : REVERB
    // ----------------------------
    else
    {
        // Mise à jour des paramètres de la reverb
        auto params = reverb.getParameters();
        params.roomSize = roomSize;
        params.wetLevel = wet;
        params.dryLevel = dry;
        reverb.setParameters(params);

        if (totalNumOutputChannels == 1)
        {
            reverb.processMono(buffer.getWritePointer(0), numSamples);
        }
        else
        {
            // On ne traite que les deux premiers canaux (classique pour une reverb)
            reverb.processStereo(buffer.getWritePointer(0),
                buffer.getWritePointer(1),
                numSamples);
        }
    }
}

//==============================================================================
// GUI
//==============================================================================

bool SimpleReverbAudioProcessor::hasEditor() const
{
    return true;
}

juce::AudioProcessorEditor* SimpleReverbAudioProcessor::createEditor()
{
    return new SimpleReverbAudioProcessorEditor(*this);
}

//==============================================================================
// Sauvegarde / restauration de l’état
//==============================================================================

void SimpleReverbAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void SimpleReverbAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml(getXmlFromBinary(data, sizeInBytes));

    if (xml.get() != nullptr && xml->hasTagName(apvts.state.getType()))
        apvts.replaceState(juce::ValueTree::fromXml(*xml));
}

//==============================================================================
// Factory : création d’une instance du plugin
//==============================================================================

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new SimpleReverbAudioProcessor();
}
