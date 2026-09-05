#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>

struct ChainSettings {
    float peakFreq{0}, peakQ{1.f}, peakGain{0};
    float lowcutFreq{0}, highcutFreq{0};
    float lowcutSlope{0}, highcutSlope{0};
};

ChainSettings getChainSettings(const juce::AudioProcessorValueTreeState &apvts);

using Filter = juce::dsp::IIR::Filter<float>;
using CutoffChain = juce::dsp::ProcessorChain<Filter, Filter, Filter, Filter>;
using MonoChain = juce::dsp::ProcessorChain<CutoffChain, Filter, CutoffChain>;

enum MonoChainPosition {
    Lowcut = 0,
    Peak,
    Highcut,
};

struct ParamIds {
    static constexpr auto PeakFreq = "peakFreq", PeakQuality = "peakQuality", PeakGain = "peakGain";
    static constexpr auto LowcutFreq = "lowcutFreq", HighcutFreq = "highcutFreq";
    static constexpr auto LowcutSlope = "lowcutSlope", HighcutSlope = "highcutSlope";
};

class SimpleEQProcessor final : public juce::AudioProcessor {
public:
    SimpleEQProcessor();

    ~SimpleEQProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;

    void releaseResources() override;

    bool isBusesLayoutSupported(const BusesLayout &layouts) const override;

    void processBlock(juce::AudioBuffer<float> &, juce::MidiBuffer &) override;

    using AudioProcessor::processBlock;

    juce::AudioProcessorEditor *createEditor() override;

    bool hasEditor() const override;

    const juce::String getName() const override;

    bool acceptsMidi() const override;

    bool producesMidi() const override;

    bool isMidiEffect() const override;

    double getTailLengthSeconds() const override;

    int getNumPrograms() override;

    int getCurrentProgram() override;

    void setCurrentProgram(int index) override;

    const juce::String getProgramName(int index) override;

    void changeProgramName(int index, const juce::String &newName) override;

    void getStateInformation(juce::MemoryBlock &destData) override;

    void setStateInformation(const void *data, int sizeInBytes) override;

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    juce::AudioProcessorValueTreeState apvts{
        *this, nullptr, "Parameters", createParameterLayout()
    };

private:
    MonoChain leftChain, rightChain;

    void updatePeakFilter(const ChainSettings &chainSettings);

    void updateLowcutFilter(const ChainSettings &chainSettings);

    void updateHighcutFilter(const ChainSettings &chainSettings);

    void updateFilters(const ChainSettings &chainSettings);

    template<int Index, typename ChainType, typename Coefficients>
    static void updateCoefficients(ChainType &leftChain, ChainType &rightChain, const Coefficients &coefficients) {
        *leftChain.template get<Index>().coefficients = *coefficients[Index];
        leftChain.template setBypassed<Index>(false);
        *rightChain.template get<Index>().coefficients = *coefficients[Index];
        rightChain.template setBypassed<Index>(false);
    }

    template<typename ChainType, typename Coefficients>
    static void updateEdgeFilter(
        const Coefficients &cutCoefficients,
        ChainType &leftCut,
        ChainType &rightCut
    ) {
        leftCut.template setBypassed<0>(true);
        leftCut.template setBypassed<1>(true);
        leftCut.template setBypassed<2>(true);
        leftCut.template setBypassed<3>(true);

        rightCut.template setBypassed<0>(true);
        rightCut.template setBypassed<1>(true);
        rightCut.template setBypassed<2>(true);
        rightCut.template setBypassed<3>(true);

        if (cutCoefficients.size() > 0) updateCoefficients<0>(leftCut, rightCut, cutCoefficients);
        if (cutCoefficients.size() > 1) updateCoefficients<1>(leftCut, rightCut, cutCoefficients);
        if (cutCoefficients.size() > 2) updateCoefficients<2>(leftCut, rightCut, cutCoefficients);
        if (cutCoefficients.size() > 3) updateCoefficients<3>(leftCut, rightCut, cutCoefficients);
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SimpleEQProcessor)
};
