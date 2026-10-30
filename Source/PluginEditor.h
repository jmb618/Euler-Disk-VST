#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include <memory>
#include "PluginProcessor.h"

class DiscView;
class TubeView;
class ProgressStrip;

// Interface minimaliste "pixel / Elektron" :
//   - en haut : LANCER, POSITION, progression
//   - 3 disques fil de fer 3D (un par voix) avec leurs 4 reglages dessous
//   - reglages globaux groupes, puis saturation + tube pixel en bas
class PixelEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit PixelEditor (EulerProcessor&);
    ~PixelEditor() override;

    void paint (juce::Graphics&) override;

private:
    void timerCallback() override;

    EulerProcessor& proc;
    juce::OwnedArray<juce::Component> comps;   // possede tous les controles

    DiscView*      discs[3] = { nullptr, nullptr, nullptr };
    TubeView*      tube     = nullptr;
    ProgressStrip* progress = nullptr;

    juce::RangedAudioParameter* diamParam[3] = { nullptr, nullptr, nullptr };
    juce::RangedAudioParameter* hrelParam = nullptr;

    double lastMs = 0.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PixelEditor)
};
