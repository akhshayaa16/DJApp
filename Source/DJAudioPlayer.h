/*
  ==============================================================================

    DJAudioPlayer.h
    Author:  Chandrasekaran Akhshayaa

  ==============================================================================
*/

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "BPMDetector.h"

class DJAudioPlayer : public AudioSource
{
public:
    DJAudioPlayer(AudioFormatManager& _formatManager);
    ~DJAudioPlayer();

    /** Prepare audio pipeline for playback */
    void prepareToPlay (int samplesPerBlockExpected, double sampleRate) override;
    /** Fill the audio buffer with the next block of samples, applying EQ */
    void getNextAudioBlock (const AudioSourceChannelInfo& bufferToFill) override;
    /** Release audio resources when no longer needed */
    void releaseResources() override;

    /** Load an audio file from a URL into the player */
    void loadURL(URL audioURL);
    /** Set the playback volume (0.0 to 1.0) */
    void setGain(double gain);
    /** Set the playback speed ratio (0.1 to 4.0, where 1.0 is normal) */
    void setSpeed(double ratio);
    /** Set the playback position in seconds */
    void setPosition(double posInSecs);
    /** Set the playback position as a fraction of total length (0.0 to 1.0) */
    void setPositionRelative(double pos);

    /** Start audio playback */
    void start();
    /** Stop audio playback */
    void stop();

    /** Get the current playback position as a fraction (0.0 to 1.0) */
    double getPositionRelative();
    /** Get the detected BPM of the loaded track (0.0 if unknown) */
    double getBpm() const { return bpm; }
    /** Check whether audio is currently playing */
    bool isPlaying() const { return transportSource.isPlaying(); }

    /** Set the low-band EQ gain in dB (-24 to +24) */
    void setLowEQGainDb (float gainDb);
    /** Set the mid-band EQ gain in dB (-24 to +24) */
    void setMidEQGainDb (float gainDb);
    /** Set the high-band EQ gain in dB (-24 to +24) */
    void setHighEQGainDb(float gainDb);

private:
    using Filter = juce::dsp::IIR::Filter<float>;
    using Coeffs = juce::dsp::IIR::Coefficients<float>;
    using Chain  = juce::dsp::ProcessorChain<Filter, Filter, Filter>;

    void updateEQCoefficients();

    enum EqBand { Low = 0, Mid = 1, High = 2 };

    Chain eqLeft;
    Chain eqRight;

    double currentSampleRate = 44100.0;
    double bpm = 0.0;

    float lowFreqHz  = 200.0f;
    float midFreqHz  = 1000.0f;
    float highFreqHz = 6000.0f;
    float midQ = 0.707f;

    float lowGainDb  = 0.0f;
    float midGainDb  = 0.0f;
    float highGainDb = 0.0f;

    AudioFormatManager& formatManager;
    std::unique_ptr<AudioFormatReaderSource> readerSource;
    AudioTransportSource transportSource;
    ResamplingAudioSource resampleSource{ &transportSource, false, 2 };
};
