#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "DJAudioPlayer.h"
#include "WaveformDisplay.h"
#include <array>

class DeckGUI : public juce::Component,
                public juce::Button::Listener,
                public juce::Slider::Listener,
                public juce::FileDragAndDropTarget,
                public juce::Timer
{
public:
    DeckGUI(DJAudioPlayer* player,
            juce::AudioFormatManager& formatManagerToUse,
            juce::AudioThumbnailCache& cacheToUse);
    ~DeckGUI() override;

    void paint(juce::Graphics&) override;
    void resized() override;

    void buttonClicked(juce::Button*) override;
    void sliderValueChanged(juce::Slider* slider) override;

    bool isInterestedInFileDrag(const juce::StringArray& files) override;
    void filesDropped(const juce::StringArray& files, int x, int y) override;

    void timerCallback() override;

    void loadFile(juce::File file);

private:
    // -------------------------
    // R3 Hot Cues
    // -------------------------
    void initHotCues();
    void clearAllHotCues();
    void updateHotCueButtonLabels();
    void loadHotCuesForCurrentTrack();
    void saveHotCuesForCurrentTrack();

    juce::File getHotCuesFile();
    juce::var loadHotCuesJson();
    void saveHotCuesJson(juce::var json);

    // -------------------------
    // R4 EQ Persistence
    // -------------------------
    void applyEQToPlayer();
    void loadEQForCurrentTrack();
    void saveEQForCurrentTrack();

    juce::File getEQFile();
    juce::var loadEQJson();
    void saveEQJson(juce::var json);

    // -------------------------
    // R5 BPM UI
    // -------------------------
    void updateBpmLabel();

private:
    juce::FileChooser fChooser { "Select a file..." };

    juce::TextButton playButton { "PLAY" };
    juce::TextButton stopButton { "STOP" };
    juce::TextButton loadButton { "LOAD" };

    juce::Slider volSlider;
    juce::Slider speedSlider;
    juce::Slider posSlider;

    WaveformDisplay waveformDisplay;

    // R4: EQ sliders (dB)
    juce::Slider lowEQSlider;
    juce::Slider midEQSlider;
    juce::Slider highEQSlider;

    // EQ Labels
    juce::Label lowEQLabel;
    juce::Label midEQLabel;
    juce::Label highEQLabel;

    // ✅ BPM label
    juce::Label bpmLabel;

    // R3 Hot Cues
    juce::ToggleButton cueModeButton { "CUE MODE" };
    juce::TextButton clearCuesButton { "CLEAR CUES" };
    std::array<juce::TextButton, 8> hotCueButtons {
        juce::TextButton{"CUE 1"}, juce::TextButton{"CUE 2"}, juce::TextButton{"CUE 3"}, juce::TextButton{"CUE 4"},
        juce::TextButton{"CUE 5"}, juce::TextButton{"CUE 6"}, juce::TextButton{"CUE 7"}, juce::TextButton{"CUE 8"}
    };

    std::array<double, 8> hotCues; // relative positions (0.0 to 1.0). -1.0 = empty

    // EQ values in dB (last adjusted)
    double lowDb  = 0.0;
    double midDb  = 0.0;
    double highDb = 0.0;

    // "Original" EQ values (baseline = 0 dB)
    double originalLowDb  = 0.0;
    double originalMidDb  = 0.0;
    double originalHighDb = 0.0;

    DJAudioPlayer* player = nullptr;
    juce::String loadedTrackPath;

    // Theme colours
    const juce::Colour bgColour      { juce::Colour(30, 35, 40) };
    const juce::Colour panelColour   { juce::Colour(40, 45, 50) };
    const juce::Colour borderColour  { juce::Colour(60, 65, 70) };
    const juce::Colour accentColour  { juce::Colour(0, 170, 255) };

    bool lastIsPlaying = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DeckGUI)
};

