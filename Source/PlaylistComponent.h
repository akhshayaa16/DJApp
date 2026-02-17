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
    /** Create the playlist, loading any previously saved library */
    PlaylistComponent(AudioFormatManager& formatManager);
    /** Save the library to disk on destruction */
    ~PlaylistComponent() override;

    /** Draw the playlist background */
    void paint (juce::Graphics&) override;
    /** Layout the add button and track table */
    void resized() override;

    /** Return the number of tracks in the playlist */
    int getNumRows() override;
    /** Draw the background for a table row */
    void paintRowBackground(Graphics & g,
                            int rowNumber,
                            int width,
                            int height,
                            bool rowIsSelected) override;

    /** Draw the text content of a table cell (name or duration) */
    void paintCell(Graphics & g,
                   int rowNumber,
                   int columnId,
                   int width,
                   int height,
                   bool rowIsSelected) override;

    /** Create or update the load-to-deck buttons in each row */
    Component* refreshComponentForCell(int rowNumber,
                                       int columnId,
                                       bool isRowSelected,
                                       Component *existingComponentToUpdate) override;

    /** Handle add-tracks button and load-to-deck button clicks */
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
