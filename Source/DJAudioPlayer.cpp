
#include "DJAudioPlayer.h"

DJAudioPlayer::DJAudioPlayer(AudioFormatManager& _formatManager)
: formatManager(_formatManager)
{
}

DJAudioPlayer::~DJAudioPlayer()
{
}

void DJAudioPlayer::prepareToPlay (int samplesPerBlockExpected, double sampleRate)
{
    currentSampleRate = sampleRate;

    transportSource.prepareToPlay(samplesPerBlockExpected, sampleRate);
    resampleSource.prepareToPlay(samplesPerBlockExpected, sampleRate);
    resampleSource.setResamplingRatio(1.0);

    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = (juce::uint32) samplesPerBlockExpected;
    spec.numChannels = 1;

    eqLeft.prepare(spec);
    eqRight.prepare(spec);
    eqLeft.reset();
    eqRight.reset();

    updateEQCoefficients();
}

void DJAudioPlayer::getNextAudioBlock (const AudioSourceChannelInfo& bufferToFill)
{
    resampleSource.getNextAudioBlock(bufferToFill);

    auto* buffer = bufferToFill.buffer;
    if (!buffer) return;

    auto block = juce::dsp::AudioBlock<float>(*buffer)
                    .getSubBlock((size_t)bufferToFill.startSample,
                                 (size_t)bufferToFill.numSamples);

    if (buffer->getNumChannels() >= 1)
    {
        auto leftBlock = block.getSingleChannelBlock(0);
        juce::dsp::ProcessContextReplacing<float> ctx(leftBlock);
        eqLeft.process(ctx);
    }

    if (buffer->getNumChannels() >= 2)
    {
        auto rightBlock = block.getSingleChannelBlock(1);
        juce::dsp::ProcessContextReplacing<float> ctx(rightBlock);
        eqRight.process(ctx);
    }
}

void DJAudioPlayer::releaseResources()
{
    transportSource.releaseResources();
    resampleSource.releaseResources();
}

void DJAudioPlayer::loadURL(URL audioURL)
{
    auto stream = audioURL.createInputStream(false);
    if (stream == nullptr)
    {
        bpm = 0.0;
        return;
    }

    auto* reader = formatManager.createReaderFor(std::move(stream));
    if (reader == nullptr)
    {
        bpm = 0.0;
        return;
    }

    // -------- BPM ANALYSIS --------
    const double sr = reader->sampleRate;
    const int numCh = (int) reader->numChannels;

    const int maxSecondsToRead = 60;
    const int maxSamplesToRead = (int) std::min<int64>(
        reader->lengthInSamples,
        (int64)(sr * maxSecondsToRead)
    );

    if (sr > 0.0 && numCh > 0 && maxSamplesToRead > 0)
    {
        juce::AudioBuffer<float> analysisBuffer(numCh, maxSamplesToRead);
        analysisBuffer.clear();

        reader->read(&analysisBuffer, 0, maxSamplesToRead, 0, true, true);

        bpm = BPMDetector::detectBpmFromBuffer(
            analysisBuffer, sr, 70.0, 200.0);
    }
    else
    {
        bpm = 0.0;
    }

    // -------- NORMAL LOADING --------
    std::unique_ptr<AudioFormatReaderSource> newSource(
        new AudioFormatReaderSource(reader, true)
    );

    transportSource.setSource(newSource.get(),
                              0,
                              nullptr,
                              reader->sampleRate);

    readerSource.reset(newSource.release());
}

void DJAudioPlayer::setGain(double gain)
{
    gain = juce::jlimit(0.0, 1.0, gain);
    transportSource.setGain(gain);
}

void DJAudioPlayer::setSpeed(double ratio)
{
    ratio = juce::jlimit(0.1, 4.0, ratio);
    resampleSource.setResamplingRatio(ratio);
}

void DJAudioPlayer::setPosition(double posInSecs)
{
    transportSource.setPosition(posInSecs);
}

void DJAudioPlayer::setPositionRelative(double pos)
{
    pos = juce::jlimit(0.0, 1.0, pos);
    double length = transportSource.getLengthInSeconds();
    if (length > 0.0)
        setPosition(length * pos);
}

void DJAudioPlayer::start()  { transportSource.start(); }
void DJAudioPlayer::stop()   { transportSource.stop(); }

double DJAudioPlayer::getPositionRelative()
{
    double len = transportSource.getLengthInSeconds();
    if (len <= 0.0) return 0.0;
    return transportSource.getCurrentPosition() / len;
}

void DJAudioPlayer::setLowEQGainDb(float gainDb)
{
    lowGainDb = juce::jlimit(-24.0f, 24.0f, gainDb);
    updateEQCoefficients();
}

void DJAudioPlayer::setMidEQGainDb(float gainDb)
{
    midGainDb = juce::jlimit(-24.0f, 24.0f, gainDb);
    updateEQCoefficients();
}

void DJAudioPlayer::setHighEQGainDb(float gainDb)
{
    highGainDb = juce::jlimit(-24.0f, 24.0f, gainDb);
    updateEQCoefficients();
}

void DJAudioPlayer::updateEQCoefficients()
{
    if (currentSampleRate <= 0.0) return;

    auto low  = Coeffs::makeLowShelf (currentSampleRate, lowFreqHz,  midQ,
                                      juce::Decibels::decibelsToGain(lowGainDb));
    auto mid  = Coeffs::makePeakFilter(currentSampleRate, midFreqHz, midQ,
                                      juce::Decibels::decibelsToGain(midGainDb));
    auto high = Coeffs::makeHighShelf(currentSampleRate, highFreqHz, midQ,
                                      juce::Decibels::decibelsToGain(highGainDb));

    *eqLeft.get<Low>().coefficients  = *low;
    *eqLeft.get<Mid>().coefficients  = *mid;
    *eqLeft.get<High>().coefficients = *high;

    *eqRight.get<Low>().coefficients  = *low;
    *eqRight.get<Mid>().coefficients  = *mid;
    *eqRight.get<High>().coefficients = *high;
}
