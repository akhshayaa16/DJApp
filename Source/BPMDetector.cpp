/*
  ==============================================================================

    BPMDetector.cpp
    Created: 15 Feb 2026 9:01:22pm
    Author:  Chandrasekaran Akhshayaa

  ==============================================================================
*/

#include "BPMDetector.h"

double BPMDetector::detectBpmFromBuffer(const juce::AudioBuffer<float>& buffer,
                                       double sampleRate,
                                       double minBpm,
                                       double maxBpm)
{
    const int numCh = buffer.getNumChannels();
    const int numSamp = buffer.getNumSamples();
    if (numCh <= 0 || numSamp <= 0 || sampleRate <= 0.0) return 0.0;

    // --- 1) Convert to mono (average channels) ---
    juce::AudioBuffer<float> mono(1, numSamp);
    mono.clear();

    if (numCh == 1)
    {
        mono.copyFrom(0, 0, buffer, 0, 0, numSamp);
    }
    else
    {
        auto* dst = mono.getWritePointer(0);
        for (int ch = 0; ch < numCh; ++ch)
        {
            auto* src = buffer.getReadPointer(ch);
            for (int i = 0; i < numSamp; ++i)
                dst[i] += src[i];
        }
        const float inv = 1.0f / (float) numCh;
        for (int i = 0; i < numSamp; ++i)
            dst[i] *= inv;
    }

    // --- 2) Build a short-time energy envelope ---
    // Use hop-based RMS so we get an “energy over time” signal suitable for tempo finding.
    const int win = 1024;   // window size in samples
    const int hop = 512;    // hop size
    if (numSamp < win + hop) return 0.0;

    const int envLen = 1 + (numSamp - win) / hop;
    juce::Array<float> env;
    env.ensureStorageAllocated(envLen);

    auto* x = mono.getReadPointer(0);

    float maxEnv = 0.0f;
    for (int frame = 0; frame < envLen; ++frame)
    {
        const int start = frame * hop;
        double sumSq = 0.0;

        for (int i = 0; i < win; ++i)
        {
            const float s = x[start + i];
            sumSq += (double) s * (double) s;
        }

        const float rms = (float) std::sqrt(sumSq / (double) win);
        env.add(rms);
        maxEnv = std::max(maxEnv, rms);
    }

    if (maxEnv <= 1.0e-6f) return 0.0;

    // --- 3) Normalize + simple smoothing (moving average) ---
    for (int i = 0; i < env.size(); ++i)
        env.set(i, env[i] / maxEnv);

    // Smooth with a small moving average (~ 5 frames)
    const int smoothN = 5;
    juce::Array<float> smooth;
    smooth.ensureStorageAllocated(env.size());
    smooth.resize(env.size());

    for (int i = 0; i < env.size(); ++i)
    {
        float acc = 0.0f;
        int cnt = 0;
        for (int k = -smoothN; k <= smoothN; ++k)
        {
            const int j = i + k;
            if (j >= 0 && j < env.size())
            {
                acc += env[j];
                cnt++;
            }
        }
        smooth.set(i, acc / (float) std::max(1, cnt));
    }

    // Remove mean (helps autocorrelation)
    double mean = 0.0;
    for (int i = 0; i < smooth.size(); ++i) mean += smooth[i];
    mean /= (double) smooth.size();

    for (int i = 0; i < smooth.size(); ++i)
        smooth.set(i, (float) (smooth[i] - (float) mean));

    // Half-wave rectify (focus on “beats” peaks)
    for (int i = 0; i < smooth.size(); ++i)
        smooth.set(i, std::max(0.0f, smooth[i]));

    // Envelope sample rate (frames per second)
    const double envRate = sampleRate / (double) hop;

    // --- 4) Autocorrelation within BPM lag range ---
    const double minHz = minBpm / 60.0;
    const double maxHz = maxBpm / 60.0;

    const int minLag = (int) std::floor(envRate / maxHz); // faster beats -> smaller lag
    const int maxLag = (int) std::ceil (envRate / minHz); // slower beats -> bigger lag

    if (maxLag >= smooth.size() - 1 || minLag < 1) return 0.0;

    int bestLag = -1;
    double bestScore = -1.0;

    for (int lag = minLag; lag <= maxLag; ++lag)
    {
        double sum = 0.0;
        for (int i = 0; i < smooth.size() - lag; ++i)
            sum += (double) smooth[i] * (double) smooth[i + lag];

        if (sum > bestScore)
        {
            bestScore = sum;
            bestLag = lag;
        }
    }

    if (bestLag <= 0 || bestScore <= 0.0) return 0.0;

    double bpm = 60.0 * envRate / (double) bestLag;

    // Basic sanity: allow half/double correction to keep in range
    while (bpm < minBpm) bpm *= 2.0;
    while (bpm > maxBpm) bpm *= 0.5;

    // Optional: round to 1 decimal
    bpm = std::round(bpm * 10.0) / 10.0;

    return bpm;
}
