#include "PluginProcessor.h"
#include "PluginEditor.h"

SimpleEQProcessor::SimpleEQProcessor()
    : AudioProcessor(BusesProperties()
#if !JucePlugin_IsMidiEffect
#if !JucePlugin_IsSynth
        .withInput("Input", juce::AudioChannelSet::stereo(), true)
#endif
        .withOutput("Output", juce::AudioChannelSet::stereo(), true)
#endif
    ) {
}

SimpleEQProcessor::~SimpleEQProcessor()
= default;

const juce::String SimpleEQProcessor::getName() const {
    return "Basic-EQ";
}

bool SimpleEQProcessor::acceptsMidi() const {
#if JucePlugin_WantsMidiInput
    return true;
#else
    return false;
#endif
}

bool SimpleEQProcessor::producesMidi() const {
#if JucePlugin_ProducesMidiOutput
    return true;
#else
    return false;
#endif
}

bool SimpleEQProcessor::isMidiEffect() const {
#if JucePlugin_IsMidiEffect
    return true;
#else
    return false;
#endif
}

double SimpleEQProcessor::getTailLengthSeconds() const {
    return 0.0;
}

int SimpleEQProcessor::getNumPrograms() {
    return 1; // NB: some hosts don't cope very well if you tell them there are 0 programs,
    // so this should be at least 1, even if you're not really implementing programs.
}

int SimpleEQProcessor::getCurrentProgram() {
    return 0;
}

void SimpleEQProcessor::setCurrentProgram(int index) {
    juce::ignoreUnused(index);
}

const juce::String SimpleEQProcessor::getProgramName(int index) {
    juce::ignoreUnused(index);
    return {};
}

void SimpleEQProcessor::changeProgramName(int index, const juce::String &newName) {
    juce::ignoreUnused(index, newName);
}

void SimpleEQProcessor::prepareToPlay(double sampleRate, int samplesPerBlock) {
    const juce::dsp::ProcessSpec spec{
        .sampleRate = sampleRate,
        .maximumBlockSize = static_cast<juce::uint32>(samplesPerBlock),
        .numChannels = 1u,
    };

    leftChain.prepare(spec);
    rightChain.prepare(spec);

    const auto chainSettings = getChainSettings(apvts);
    updateFilters(chainSettings);
}

void SimpleEQProcessor::releaseResources() {
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

bool SimpleEQProcessor::isBusesLayoutSupported(const BusesLayout &layouts) const {
#if JucePlugin_IsMidiEffect
    juce::ignoreUnused(layouts);
    return true;
#else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono() && layouts.getMainOutputChannelSet() !=
        juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
#if !JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
#endif

    return true;
#endif
}

void SimpleEQProcessor::processBlock(juce::AudioBuffer<float> &buffer, juce::MidiBuffer &midiMessages) {
    juce::ignoreUnused(midiMessages);

    juce::ScopedNoDenormals noDenormals;
    const auto totalNumInputChannels = getTotalNumInputChannels();
    const auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear(i, 0, buffer.getNumSamples());

    const auto chainSettings = getChainSettings(apvts);
    updateFilters(chainSettings);

    const juce::dsp::AudioBlock<float> block(buffer);
    auto leftBlock = block.getSingleChannelBlock(0);
    auto rightBlock = block.getSingleChannelBlock(1);
    const juce::dsp::ProcessContextReplacing<float> leftContext{leftBlock};
    const juce::dsp::ProcessContextReplacing<float> rightContext{rightBlock};

    leftChain.process(leftContext);
    rightChain.process(rightContext);
}

bool SimpleEQProcessor::hasEditor() const {
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor *SimpleEQProcessor::createEditor() {
    return new AudioPluginAudioProcessorEditor(*this);
    // return new juce::GenericAudioProcessorEditor(*this);
}

void SimpleEQProcessor::getStateInformation(juce::MemoryBlock &destData) {
    juce::MemoryOutputStream outputStream{destData, true};
    apvts.state.writeToStream(outputStream);
}

void SimpleEQProcessor::setStateInformation(const void *data, int sizeInBytes) {
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
    if (
        const auto tree = juce::ValueTree::readFromData(data, static_cast<size_t>(sizeInBytes));
        tree.isValid()
    )
        apvts.replaceState(tree);
}

ChainSettings getChainSettings(const juce::AudioProcessorValueTreeState &apvts) {
    const ChainSettings settings{
        .peakFreq = apvts.getRawParameterValue(ParamIds::PeakFreq)->load(),
        .peakQ = apvts.getRawParameterValue(ParamIds::PeakQuality)->load(),
        .peakGain = apvts.getRawParameterValue(ParamIds::PeakGain)->load(),
        .lowcutFreq = apvts.getRawParameterValue(ParamIds::LowcutFreq)->load(),
        .highcutFreq = apvts.getRawParameterValue(ParamIds::HighcutFreq)->load(),
        .lowcutSlope = apvts.getRawParameterValue(ParamIds::LowcutSlope)->load(),
        .highcutSlope = apvts.getRawParameterValue(ParamIds::HighcutSlope)->load(),
    };
    return settings;
}

void SimpleEQProcessor::updateFilters(const ChainSettings &chainSettings) {
    updateLowcutFilter(chainSettings);
    updatePeakFilter(chainSettings);
    updateHighcutFilter(chainSettings);
}

void SimpleEQProcessor::updatePeakFilter(const ChainSettings &chainSettings) {
    const auto peakCoefficients = juce::dsp::IIR::Coefficients<float>::makePeakFilter(
        getSampleRate(),
        chainSettings.peakFreq,
        chainSettings.peakQ,
        juce::Decibels::decibelsToGain(chainSettings.peakGain)
    );
    *leftChain.get<MonoChainPosition::Peak>().coefficients = *peakCoefficients;
    *rightChain.get<MonoChainPosition::Peak>().coefficients = *peakCoefficients;
}

void SimpleEQProcessor::updateLowcutFilter(const ChainSettings &chainSettings) {
    const auto lowcutCoefficients = juce::dsp::FilterDesign<float>::designIIRHighpassHighOrderButterworthMethod(
        chainSettings.lowcutFreq,
        getSampleRate(),
        static_cast<int>(chainSettings.lowcutSlope) / 6
    );
    auto &leftLowcut = leftChain.get<MonoChainPosition::Lowcut>();
    auto &rightLowcut = rightChain.get<MonoChainPosition::Lowcut>();
    updateEdgeFilter(lowcutCoefficients, leftLowcut, rightLowcut);
}

void SimpleEQProcessor::updateHighcutFilter(const ChainSettings &chainSettings) {
    const auto highcutCoefficients = juce::dsp::FilterDesign<float>::designIIRLowpassHighOrderButterworthMethod(
        chainSettings.highcutFreq,
        getSampleRate(),
        static_cast<int>(chainSettings.highcutSlope) / 6
    );
    auto &leftHighcut = leftChain.get<MonoChainPosition::Highcut>();
    auto &rightHighcut = rightChain.get<MonoChainPosition::Highcut>();
    updateEdgeFilter(highcutCoefficients, leftHighcut, rightHighcut);
}

juce::AudioProcessorValueTreeState::ParameterLayout SimpleEQProcessor::createParameterLayout() {
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    constexpr auto versionHint = 1;
    layout.add(
        std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{ParamIds::LowcutFreq, versionHint},
            "Lowcut Frequency",
            juce::NormalisableRange<float>{20.f, 20000.f, 1.f, 0.25f},
            20.f
        )
    );
    layout.add(
        std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{ParamIds::HighcutFreq, versionHint},
            "Highcut Frequency",
            juce::NormalisableRange<float>{20.f, 20000.f, 1.f, 0.25f},
            20000.f
        )
    );
    layout.add(
        std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{ParamIds::PeakFreq, versionHint},
            "Peak Frequency",
            juce::NormalisableRange<float>{20.f, 20000.f, 1.f, 0.25f},
            750.f
        )
    );
    layout.add(
        std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{ParamIds::PeakGain, versionHint},
            "Peak Gain",
            juce::NormalisableRange<float>{-24.f, 24.f, 0.2f, 1.f},
            0.f,
            juce::AudioParameterFloatAttributes{}.withLabel("dB")
        )
    );
    layout.add(
        std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{ParamIds::PeakQuality, versionHint},
            "Peak Q",
            juce::NormalisableRange<float>{0.1f, 10.f, 0.05f, 1.f},
            1.f
        )
    );
    layout.add(
        std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{ParamIds::LowcutSlope, versionHint},
            "Lowcut Slope",
            juce::NormalisableRange<float>{6.f, 48.f, 6.f, 1.f},
            24.f,
            juce::AudioParameterFloatAttributes{}.withLabel("dB/oct")
        )
    );
    layout.add(
        std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{ParamIds::HighcutSlope, versionHint},
            "Highcut Slope",
            juce::NormalisableRange<float>{6.f, 48.f, 6.f, 1.f},
            24.f,
            juce::AudioParameterFloatAttributes{}.withLabel("dB/oct")
        )
    );
    return layout;
}

// This creates new instances of the plugin..
juce::AudioProcessor *JUCE_CALLTYPE createPluginFilter() {
    return new SimpleEQProcessor();
}
