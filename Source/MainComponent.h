/*
  ==============================================================================

    MainComponent.h
    Author:  Chandrasekaran Akhshayaa

  ==============================================================================
*/

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "DJAudioPlayer.h"
#include "DeckGUI.h"
#include "PlaylistComponent.h"

//==============================================================================
class MainComponent  : public AudioAppComponent
{
public:
    MainComponent();
    ~MainComponent() override;

    /** Prepare both audio players and the mixer for playback */
    void prepareToPlay (int samplesPerBlockExpected, double sampleRate) override;
    /** Mix both deck audio sources into the output buffer */
    void getNextAudioBlock (const AudioSourceChannelInfo& bufferToFill) override;
    /** Release audio resources for both players and the mixer */
    void releaseResources() override;

    /** Fill the background with the application theme colour */
    void paint (Graphics& g) override;
    /** Layout the two decks side-by-side with the playlist below */
    void resized() override;

private:
    AudioFormatManager formatManager;
    AudioThumbnailCache thumbCache { 100 };

    DJAudioPlayer player1 { formatManager };
    DeckGUI deckGUI1 { &player1, formatManager, thumbCache };

    DJAudioPlayer player2 { formatManager };
    DeckGUI deckGUI2 { &player2, formatManager, thumbCache };

    MixerAudioSource mixerSource;
    PlaylistComponent playlistComponent { formatManager };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};

