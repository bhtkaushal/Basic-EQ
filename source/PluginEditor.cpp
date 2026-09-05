#include "PluginProcessor.h"
#include "PluginEditor.h"

AudioPluginAudioProcessorEditor::AudioPluginAudioProcessorEditor(SimpleEQProcessor &p)
    : AudioProcessorEditor(&p), processorRef(p),
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

AudioPluginAudioProcessorEditor::~AudioPluginAudioProcessorEditor() = default;

void AudioPluginAudioProcessorEditor::paint(juce::Graphics &g) {
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    using namespace juce;
    g.fillAll(Colours::dimgrey);
    auto bounds = getBounds();
    auto responseArea = bounds.removeFromTop(bounds.getHeight() * 0.6);

}

void AudioPluginAudioProcessorEditor::resized() {
    // This is generally where you'll want to lay out the positions of any
    // subcomponents in your editor..
    auto bounds = getLocalBounds();
    bounds.removeFromTop(bounds.getHeight() * 0.6); // responseArea;

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

std::vector<juce::Component *> AudioPluginAudioProcessorEditor::getComps() {
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
