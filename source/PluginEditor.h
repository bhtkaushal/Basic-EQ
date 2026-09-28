#pragma once

#include "PluginProcessor.h"

struct RotatorySlider : juce::Slider {
    RotatorySlider()
        : Slider(RotaryHorizontalVerticalDrag, NoTextBox) {
    }
};

class SimpleEQProcessorEditor final : public juce::AudioProcessorEditor, juce::AudioProcessorParameter::Listener, juce::Timer {
public:
    explicit SimpleEQProcessorEditor(SimpleEQProcessor &);
    ~SimpleEQProcessorEditor() override;
    void paint(juce::Graphics&) override;
    void resized() override;

    void parameterValueChanged(int parameterIndex, float newValue) override;
    void parameterGestureChanged(int parameterIndex, bool gestureIsStarting) override { }
    void timerCallback() override;
private:
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    SimpleEQProcessor &processorRef;
    juce::Atomic<bool> parameterChanged { false };

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

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SimpleEQProcessorEditor)
};
