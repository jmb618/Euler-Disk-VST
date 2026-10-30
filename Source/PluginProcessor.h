#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include <atomic>
#include <memory>
#include <string>
#include <utility>
#include <vector>

class EulerProcessor : public juce::AudioProcessor
{
public:
    EulerProcessor();
    ~EulerProcessor() override;

    // --- visualisation (lecture thread GUI, valeurs issues des bargraphs Faust)
    struct Viz { float omega = 0, tilt = 1, live = 0, prog = 0; };
    Viz   getViz (int voice) const;            // voice = 0..2
    float getGlobalProgress() const;

    // --- parametres : (chemin Faust, parametre)
    using ParamEntry = std::pair<std::string, juce::RangedAudioParameter*>;
    const std::vector<ParamEntry>& getParamEntries() const { return paramPaths; }
    static float actualValue (const juce::RangedAudioParameter* p);

    // --- facteur de ralentissement de l'affichage (index dans une table de l'editeur)
    int  getSlowIndex() const { return slowIndex.load(); }
    void setSlowIndex (int i) { slowIndex.store (i); }

    // --- AudioProcessor
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout&) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Euler Disk"; }
    bool acceptsMidi() const override  { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 4.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

private:
    struct Impl;
    std::unique_ptr<Impl> impl;
    std::vector<ParamEntry> paramPaths;
    std::atomic<int> slowIndex { 3 };          // 1/20 par defaut

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EulerProcessor)
};
