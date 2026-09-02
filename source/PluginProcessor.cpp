#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "juce_graphics/fonts/harfbuzz/hb-aat-layout-morx-table.hh"

SimpleEQProcessor::SimpleEQProcessor()
    : AudioProcessor(BusesProperties()
    #if !JucePlugin_IsMidiEffect
    #if !JucePlugin_IsSynth
    .withInput("Input", juce::AudioChannelSet::stereo(), true)
    #endif
    .withOutput("Output", juce::AudioChannelSet::stereo(), true)
    #endif
)
{
}

SimpleEQProcessor::~SimpleEQProcessor()
= default;

const juce::String SimpleEQProcessor::getName() const
{
    return "Basic-EQ";
}

bool SimpleEQProcessor::acceptsMidi() const
{
#if JucePlugin_WantsMidiInput
    return true;
#else
    return false;
#endif
}

bool SimpleEQProcessor::producesMidi() const
{
#if JucePlugin_ProducesMidiOutput
    return true;
#else
    return false;
#endif
}

bool SimpleEQProcessor::isMidiEffect() const
{
#if JucePlugin_IsMidiEffect
    return true;
#else
    return false;
#endif
}

double SimpleEQProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int SimpleEQProcessor::getNumPrograms()
{
    return 1; // NB: some hosts don't cope very well if you tell them there are 0 programs,
              // so this should be at least 1, even if you're not really implementing programs.
}

int SimpleEQProcessor::getCurrentProgram()
{
    return 0;
}

void SimpleEQProcessor::setCurrentProgram(int index)
{
    juce::ignoreUnused(index);
}

const juce::String SimpleEQProcessor::getProgramName(int index)
{
    juce::ignoreUnused(index);
    return {};
}

void SimpleEQProcessor::changeProgramName(int index, const juce::String &newName)
{
    juce::ignoreUnused(index, newName);
}

void SimpleEQProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    // Use this method as the place to do any pre-playback
    // initialisation that you need..
    const juce::dsp::ProcessSpec spec {
        .sampleRate = sampleRate,
        .maximumBlockSize = static_cast<juce::uint32>(samplesPerBlock),
        .numChannels = 1u,
    };

    leftChain.prepare(spec);
    rightChain.prepare(spec);

    updatePeakCoefficients();
    // auto lowCutCoefficients = juce::dsp::IIR::Coefficients<float>::makeHighPass(
    //     sampleRate,
    //     chainSettings.lowCut
    // );
    // auto highCutCoefficients = juce::dsp::IIR::Coefficients<float>::makeLowPass(
    //     sampleRate,
    //     chainSettings.highCut
    // );
}

void SimpleEQProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

bool SimpleEQProcessor::isBusesLayoutSupported(const BusesLayout &layouts) const
{
#if JucePlugin_IsMidiEffect
    juce::ignoreUnused(layouts);
    return true;
#else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono() && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
#if !JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
#endif

    return true;
#endif
}

void SimpleEQProcessor::processBlock(juce::AudioBuffer<float> &buffer, juce::MidiBuffer &midiMessages)
{
    juce::ignoreUnused(midiMessages);

    juce::ScopedNoDenormals noDenormals;
    const auto totalNumInputChannels = getTotalNumInputChannels();
    const auto totalNumOutputChannels = getTotalNumOutputChannels();

    // In case we have more outputs than inputs, this code clears any output
    // channels that didn't contain input data, (because these aren't
    // guaranteed to be empty - they may contain garbage).
    // This is here to avoid people getting screaming feedback
    // when they first compile a plugin, but obviously you don't need to keep
    // this code if your algorithm always overwrites all the output channels.
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear(i, 0, buffer.getNumSamples());

    updatePeakCoefficients();

    const juce::dsp::AudioBlock<float> block(buffer);
    auto leftBlock = block.getSingleChannelBlock(0);
    auto rightBlock = block.getSingleChannelBlock(1);
    const juce::dsp::ProcessContextReplacing<float> leftContext {leftBlock};
    const juce::dsp::ProcessContextReplacing<float> rightContext {rightBlock};

    leftChain.process(leftContext);
    rightChain.process(rightContext);
}

bool SimpleEQProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor *SimpleEQProcessor::createEditor()
{
    return new AudioPluginAudioProcessorEditor(*this);
    // return new juce::GenericAudioProcessorEditor(*this);
}

void SimpleEQProcessor::getStateInformation(juce::MemoryBlock &destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
    juce::ignoreUnused(destData);
}

void SimpleEQProcessor::setStateInformation(const void *data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
    juce::ignoreUnused(data, sizeInBytes);
}

ChainSettings getChainSettings(juce::AudioProcessorValueTreeState &apvts) {
    ChainSettings settings{
        .peakFreq = apvts.getRawParameterValue("peak")->load(),
        .peakQ = apvts.getRawParameterValue("peakQuality")->load(),
        .peakGain = apvts.getRawParameterValue("peakGain")->load(),
        .lowCut = apvts.getRawParameterValue("lowCut")->load(),
        .highCut = apvts.getRawParameterValue("highCut")->load(),
        .lowCutSlope = apvts.getRawParameterValue("lowCutSlope")->load(),
        .highCutSlope = apvts .getRawParameterValue("highCutSlope")->load(),
    };
    return settings;
}

void SimpleEQProcessor::updatePeakCoefficients() {
    auto chainSettings = getChainSettings(apvts);
    auto peakCoefficients= juce::dsp::IIR::Coefficients<float>::makePeakFilter(
        getSampleRate(),
        chainSettings.peakFreq,
        chainSettings.peakQ,
        juce::Decibels::decibelsToGain(chainSettings.peakGain)
    );
    *leftChain.get<MonoPosition::Peak>().coefficients = *peakCoefficients;
    *rightChain.get<MonoPosition::Peak>().coefficients = *peakCoefficients;
}

juce::AudioProcessorValueTreeState::ParameterLayout SimpleEQProcessor::createParameterLayout() {
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    constexpr auto versionHint = 1;
    layout.add(
      std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID {"lowCut", versionHint},
        "LowCut Frequency",
        juce::NormalisableRange<float> {20.f, 20000.f, 1.f, 0.25f},
        20.f
      )
    );
    layout.add(
      std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID {"highCut", versionHint},
        "HighCut Frequency",
        juce::NormalisableRange<float> {20.f, 20000.f, 1.f, 0.25f},
        20000.f
      )
    );
    layout.add(
      std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID {"peak", versionHint},
        "Peak Frequency",
        juce::NormalisableRange<float> {20.f, 20000.f, 1.f, 0.25f},
        750.f
      )
    );
    layout.add(
      std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID {"peakGain", versionHint},
        "Peak Gain",
        juce::NormalisableRange<float> {-24.f, 24.f, 0.2f, 1.f},
        0.f,
        juce::AudioParameterFloatAttributes {}.withLabel("dB")
      )
    );
    layout.add(
      std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID {"peakQuality", versionHint},
        "Peak Q",
        juce::NormalisableRange<float> {0.1f, 10.f, 0.05f, 1.f},
        1.f
      )
    );
    layout.add(
      std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID {"lowCutSlope", versionHint},
        "LowCut Slope",
        juce::NormalisableRange<float> {6.f, 48.f, 6.f, 1.f},
        24.f,
        juce::AudioParameterFloatAttributes {}.withLabel("dB/oct")
      )
      );
    layout.add(
      std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID {"highCutSlope", versionHint},
        "HighCut Slope",
        juce::NormalisableRange<float> {6.f, 48.f, 6.f, 1.f},
        24.f,
        juce::AudioParameterFloatAttributes {}.withLabel("dB/oct")
      )
    );
    return layout;
}

// This creates new instances of the plugin..
juce::AudioProcessor *JUCE_CALLTYPE createPluginFilter()
{
    return new SimpleEQProcessor();
}
