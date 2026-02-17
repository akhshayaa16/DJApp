/*
  ==============================================================================

    BPMDetector.h
    Created: 15 Feb 2026 9:01:22pm
    Author:  Chandrasekaran Akhshayaa

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>

class BPMDetector
{
public:
    /** Estimate BPM from an audio buffer using autocorrelation.
        Returns 0.0 if the tempo cannot be determined. */
    static double detectBpmFromBuffer(const juce::AudioBuffer<float>& buffer,
                                     double sampleRate,
                                     double minBpm = 70.0,
                                     double maxBpm = 200.0);
};
