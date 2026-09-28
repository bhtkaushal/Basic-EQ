#include "PluginProcessor.h"
#include "PluginEditor.h"

#include <ranges>

SimpleEQProcessorEditor::SimpleEQProcessorEditor(SimpleEQProcessor &processor)
    : AudioProcessorEditor(&processor), processorRef(processor),
      peakFreqSliderAttachment(processorRef.apvts, ParamIds::PeakFreq, peakFreqSlider),
      peakQualitySliderAttachment(processorRef.apvts, ParamIds::PeakQuality, peakQualitySlider),
      peakGainSliderAttachment(processorRef.apvts, ParamIds::PeakGain, peakGainSlider),
      lowcutFreqSliderAttachment(processorRef.apvts, ParamIds::LowcutFreq, lowcutFreqSlider),
      lowcutSlopeSliderAttachment(processorRef.apvts, ParamIds::LowcutSlope, lowcutSlopeSlider),
      highcutFreqSliderAttachment(processorRef.apvts, ParamIds::HighcutFreq, highcutFreqSlider),
      highcutSlopeSliderAttachment(processorRef.apvts, ParamIds::HighcutSlope, highcutSlopeSlider) {
    juce::ignoreUnused(processorRef);
    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.
    for (auto *comp: getComps()) {
        addAndMakeVisible(comp);
    }
    setSize(600, 700);
}

SimpleEQProcessorEditor::~SimpleEQProcessorEditor() = default;
 
void SimpleEQProcessorEditor::paint(juce::Graphics &g) {
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    using namespace juce;
    g.fillAll(Colours::dimgrey);
    auto bounds = getLocalBounds();
    const auto responseArea = bounds.removeFromTop(bounds.getHeight() * 0.6);
    const auto width = responseArea.getWidth();
    std::vector<double> magnitudes;
    magnitudes.resize(width);

    const auto sampleRate = processor.getSampleRate();
    auto& lowcut = monoChain.get<MonoChainPosition::Lowcut>();
    const auto& peak = monoChain.get<MonoChainPosition::Peak>();
    auto& highcut = monoChain.get<MonoChainPosition::Highcut>();

    for (const auto i: std::views::iota(0, width)) {
        auto mag = 1.f;
        const auto freq = mapToLog10<double>(static_cast<double>(i)/static_cast<double>(width), 20.0, 20000.0);

        if (!monoChain.isBypassed<MonoChainPosition::Peak>())
            mag *= peak.coefficients->getMagnitudeForFrequency(freq, sampleRate);

        if (!lowcut.isBypassed<0>())
            mag *= lowcut.get<0>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
        if (!lowcut.isBypassed<1>())
            mag *= lowcut.get<1>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
        if (!lowcut.isBypassed<2>())
            mag *= lowcut.get<2>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
        if (!lowcut.isBypassed<3>())
            mag *= lowcut.get<3>().coefficients->getMagnitudeForFrequency(freq, sampleRate);

        if (!highcut.isBypassed<0>())
            mag *= highcut.get<0>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
        if (!highcut.isBypassed<1>())
            mag *= highcut.get<1>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
        if (!highcut.isBypassed<2>())
            mag *= highcut.get<2>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
        if (!highcut.isBypassed<3>())
            mag *= highcut.get<3>().coefficients->getMagnitudeForFrequency(freq, sampleRate);

        magnitudes[i] = Decibels::gainToDecibels(mag);
    }

    Path responseCurve;
    const double outputMin = responseArea.getBottom();
    const double outputMax = responseArea.getY();
    auto map = [outputMin, outputMax](const double input) {
        return jmap<double>(input, -24.f, +24.f, outputMin, outputMax);
    };
    responseCurve.startNewSubPath(static_cast<float> (responseArea.getX()), static_cast<float> (map(magnitudes.front())));
    for (size_t i = 1; i < magnitudes.size(); ++i) {
        responseCurve.lineTo(responseArea.getX() + i, static_cast<float> (map(magnitudes[i])));
    }
    g.setColour(Colours::orange);
    g.drawRoundedRectangle(responseArea.toFloat(), 4.f, 1.f);
    g.setColour (Colours::white);
    g.strokePath(responseCurve, PathStrokeType(2.f));
}

void SimpleEQProcessorEditor::resized() {
    // This is generally where you'll want to lay out the positions  of any
    // subcomponents in your editor..
    auto bounds = getLocalBounds();
    auto responseArea = bounds.removeFromTop(bounds.getHeight() * 0.6); // responseArea;

    auto controlArea = bounds;
    const auto controlHeight = static_cast<float>(bounds.getHeight());
    const auto columnWidth = controlArea.getWidth() / 3;

    auto gainArea = controlArea.removeFromTop(controlHeight * 0.33f);
    auto slopeArea = controlArea.removeFromTop(controlHeight * 0.33f);
    auto freqArea = controlArea.removeFromBottom(controlHeight * 0.33f);

    const auto peakGainArea = gainArea.removeFromLeft(columnWidth * 2).removeFromRight(columnWidth);

    const auto lowSlopeArea = slopeArea.removeFromLeft(columnWidth);
    const auto peakQualityArea = slopeArea.removeFromLeft(columnWidth);
    const auto highSlopeArea = slopeArea.removeFromLeft(columnWidth);

    const auto lowFreqArea = freqArea.removeFromLeft(columnWidth);
    const auto peakFreqArea = freqArea.removeFromLeft(columnWidth);
    const auto highFreqArea = freqArea.removeFromLeft(columnWidth);

    lowcutFreqSlider.setBounds(lowFreqArea);
    peakFreqSlider.setBounds(peakFreqArea);
    highcutFreqSlider.setBounds(highFreqArea);
    lowcutSlopeSlider.setBounds(lowSlopeArea);
    peakQualitySlider.setBounds(peakQualityArea);
    highcutSlopeSlider.setBounds(highSlopeArea);
    peakGainSlider.setBounds(peakGainArea);


    // JUCE_LIVE_CONSTANT(true);
}

void SimpleEQProcessorEditor::parameterValueChanged(int parameterIndex, float newValue) {
    parameterChanged.set(true);
}

void SimpleEQProcessorEditor::timerCallback() {
    if (parameterChanged.compareAndSetBool(false, true)) {

    }
}

std::vector<juce::Component *> SimpleEQProcessorEditor::getComps() {
    return {
        &peakFreqSlider,
        &peakQualitySlider,
        &peakGainSlider,
        &lowcutFreqSlider,
        &lowcutSlopeSlider,
        &highcutFreqSlider,
        &highcutSlopeSlider,
    };
}
