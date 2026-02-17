///*
//  ==============================================================================
//
//    DJAudioPlayer.h
//    Created: 13 Mar 2020 4:22:22pm
//    Author:  matthew
//
//  ==============================================================================
//*/
//
//#pragma once
//
//#include "../JuceLibraryCode/JuceHeader.h"
//#include "BPMDetector.h"
//
//class DJAudioPlayer : public AudioSource
//{
//public:
//    DJAudioPlayer(AudioFormatManager& _formatManager);
//    ~DJAudioPlayer();
//
//    void prepareToPlay (int samplesPerBlockExpected, double sampleRate) override;
//    void getNextAudioBlock (const AudioSourceChannelInfo& bufferToFill) override;
//    void releaseResources() override;
//
//    void loadURL(URL audioURL);
//    void setGain(double gain);
//
//    void setSpeed(double ratio);
//    void setPosition(double posInSecs);
//    void setPositionRelative(double pos);
//
//    void start();
//    void stop();
//
//    double getPositionRelative();
//
//    // ✅ UI helper (does not affect audio logic)
//    bool isPlaying() const { return transportSource.isPlaying(); }
//
//    // =========================
//    // R4: 3-band EQ controls
//    // =========================
//    void setLowEQGainDb (float gainDb);
//    void setMidEQGainDb (float gainDb);
//    void setHighEQGainDb(float gainDb);
//
//    float getLowEQGainDb()  const { return lowGainDb; }
//    float getMidEQGainDb()  const { return midGainDb; }
//    float getHighEQGainDb() const { return highGainDb; }
//    
//    double getBpm() const { return bpm; }
//
//private:
//    using Filter = juce::dsp::IIR::Filter<float>;
//    using Coeffs = juce::dsp::IIR::Coefficients<float>;
//    using Chain  = juce::dsp::ProcessorChain<Filter, Filter, Filter>;
//
//    void updateEQCoefficients();
//
//    enum EqBand { Low = 0, Mid = 1, High = 2 };
//
//    Chain eqLeft;
//    Chain eqRight;
//
//    double currentSampleRate = 44100.0;
//
//    float lowFreqHz  = 200.0f;
//    float midFreqHz  = 1000.0f;
//    float highFreqHz = 6000.0f;
//
//    float midQ = 0.707f;
//
//    float lowGainDb  = 0.0f;
//    float midGainDb  = 0.0f;
//    float highGainDb = 0.0f;
//
//private:
//    AudioFormatManager& formatManager;
//    std::unique_ptr<AudioFormatReaderSource> readerSource;
//    AudioTransportSource transportSource;
//    ResamplingAudioSource resampleSource{&transportSource, false, 2};
//    
//private:
//    double bpm = 0.0;
//};
//


#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "BPMDetector.h"

class DJAudioPlayer : public AudioSource
{
public:
    DJAudioPlayer(AudioFormatManager& _formatManager);
    ~DJAudioPlayer();

    void prepareToPlay (int samplesPerBlockExpected, double sampleRate) override;
    void getNextAudioBlock (const AudioSourceChannelInfo& bufferToFill) override;
    void releaseResources() override;

    void loadURL(URL audioURL);
    void setGain(double gain);
    void setSpeed(double ratio);
    void setPosition(double posInSecs);
    void setPositionRelative(double pos);

    void start();
    void stop();

    double getPositionRelative();
    double getBpm() const { return bpm; }

    bool isPlaying() const { return transportSource.isPlaying(); }

    // EQ
    void setLowEQGainDb (float gainDb);
    void setMidEQGainDb (float gainDb);
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
