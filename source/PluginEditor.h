#pragma once

#include "PluginProcessor.h"

struct RotatorySlider : juce::Slider {
    RotatorySlider()
        : Slider(RotaryHorizontalVerticalDrag, NoTextBox) {
    }
};

class AudioPluginAudioProcessorEditor final : public juce::AudioProcessorEditor {
public:
    explicit AudioPluginAudioProcessorEditor(SimpleEQProcessor &);

    ~AudioPluginAudioProcessorEditor() override;

    void paint(juce::Graphics &) override;

    void resized() override;

private:
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    SimpleEQProcessor &processorRef;

    RotatorySlider
            peakFreqSlider,
            peakQualitySlider,
            peakGainSlider,
            lowcutFreqSlider,
            lowcutSlopeSlider,
            highcutFreqSlider,
            highcutSlopeSlider;

    std::vector<juce::Component *> getComps();

    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    SliderAttachment
            peakFreqSliderAttachment,
            peakQualitySliderAttachment,
            peakGainSliderAttachment,
            lowcutFreqSliderAttachment,
            lowcutSlopeSliderAttachment,
            highcutFreqSliderAttachment,
            highcutSlopeSliderAttachment;

    MonoChain monoChain; 


    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioPluginAudioProcessorEditor)
};
