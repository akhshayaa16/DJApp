/*
  ==============================================================================

    MainComponent.cpp
    Author:  Chandrasekaran Akhshayaa

  ==============================================================================
*/

#include "MainComponent.h"

MainComponent::MainComponent()
{
    setSize (800, 600);

    if (RuntimePermissions::isRequired (RuntimePermissions::recordAudio)
        && ! RuntimePermissions::isGranted (RuntimePermissions::recordAudio))
    {
        RuntimePermissions::request (RuntimePermissions::recordAudio,
                                     [&] (bool granted) { if (granted)  setAudioChannels (2, 2); });
    }
    else
    {
        setAudioChannels (0, 2);
    }

    addAndMakeVisible(deckGUI1);
    addAndMakeVisible(deckGUI2);
    addAndMakeVisible(playlistComponent);

    playlistComponent.loadToDeck1 = [this](File file) { deckGUI1.loadFile(file); };
    playlistComponent.loadToDeck2 = [this](File file) { deckGUI2.loadFile(file); };

    formatManager.registerBasicFormats();
}

MainComponent::~MainComponent()
{
    shutdownAudio();
}

void MainComponent::prepareToPlay (int samplesPerBlockExpected, double sampleRate)
{
    player1.prepareToPlay(samplesPerBlockExpected, sampleRate);
    player2.prepareToPlay(samplesPerBlockExpected, sampleRate);

    mixerSource.prepareToPlay(samplesPerBlockExpected, sampleRate);

    mixerSource.addInputSource(&player1, false);
    mixerSource.addInputSource(&player2, false);
}

void MainComponent::getNextAudioBlock (const AudioSourceChannelInfo& bufferToFill)
{
    mixerSource.getNextAudioBlock(bufferToFill);
}

void MainComponent::releaseResources()
{
    player1.releaseResources();
    player2.releaseResources();
    mixerSource.releaseResources();
}

void MainComponent::paint (Graphics& g)
{
    // ✅ Match the professional theme base colour (not default LookAndFeel)
    g.fillAll(juce::Colour(30, 35, 40));
}

void MainComponent::resized()
{
    auto area = getLocalBounds();

    // Give more space to decks (top), less to playlist (bottom)
    // Tweak this ratio if you want: 0.65 -> decks get 65% height
    const float decksRatio = 0.65f;

    const int decksH = (int) std::round(area.getHeight() * decksRatio);
    auto decksArea = area.removeFromTop(decksH);
    auto playlistArea = area; // remaining

    // Two decks side-by-side in the decksArea
    auto leftDeck  = decksArea.removeFromLeft(decksArea.getWidth() / 2);
    auto rightDeck = decksArea;

    deckGUI1.setBounds(leftDeck.reduced(4));
    deckGUI2.setBounds(rightDeck.reduced(4));

    // Playlist gets the rest (pushed down)
    playlistComponent.setBounds(playlistArea.reduced(4));
}


