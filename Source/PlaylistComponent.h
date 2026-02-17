/*
  ==============================================================================

    PlaylistComponent.h
    Created: 4 Feb 2026 4:28:33pm
    Author:  Chandrasekaran Akhshayaa

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include <vector>
#include <string>
#include <functional>


//==============================================================================

class PlaylistComponent  : public juce::Component,
                            public TableListBoxModel,
                            public Button::Listener
{
public:
    PlaylistComponent(AudioFormatManager& formatManager);
    ~PlaylistComponent() override;

    void paint (juce::Graphics&) override;
    void resized() override;

    int getNumRows() override;
    void paintRowBackground(Graphics & g,
                            int rowNumber,
                            int width,
                            int height,
                            bool rowIsSelected) override;

    void paintCell(Graphics & g,
                   int rowNumber,
                   int columnId,
                   int width,
                   int height,
                   bool rowIsSelected) override;

    Component* refreshComponentForCell(int rowNumber,
                                       int columnId,
                                       bool isRowSelected,
                                       Component *existingComponentToUpdate) override;

    void buttonClicked(Button * button) override;

    // R2B: MainComponent will set these callbacks
    std::function<void(File)> loadToDeck1;
    std::function<void(File)> loadToDeck2;

private:
    struct TrackInfo
    {
        String filePath;
        String fileName;
        double durationSec;
    };

    AudioFormatManager& formatManager;

    TableListBox tableComponent;
    TextButton addButton{"ADD TRACKS"};
    
    juce::FileChooser fChooser{"Select audio files...", File{}, "*.mp3;*.wav;*.aiff"};

    std::vector<TrackInfo> tracks;

    // R2C persistence helpers
    File getLibraryFile();
    void loadLibrary();
    void saveLibrary();

    // R2A duration helper
    double getTrackDurationSec(File file);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PlaylistComponent)
};
