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
    // Returns BPM (e.g. 128). Returns 0.0 if cannot estimate confidently.
    static double detectBpmFromBuffer(const juce::AudioBuffer<float>& buffer,
                                     double sampleRate,
                                     double minBpm = 70.0,
                                     double maxBpm = 200.0);
};
