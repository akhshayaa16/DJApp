/*
  ==============================================================================

    WaveformDisplay.h
    Created: 14 Mar 2020 3:50:16pm
    Author:  matthew

  ==============================================================================
*/

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"

//==============================================================================
/*
*/
class WaveformDisplay    : public Component, 
                           public ChangeListener
{
public:
    /** Create a waveform display using the given format manager and cache */
    WaveformDisplay( AudioFormatManager & 	formatManagerToUse,
                    AudioThumbnailCache & 	cacheToUse );
    ~WaveformDisplay();

    /** Draw the waveform thumbnail and playhead position */
    void paint (Graphics&) override;
    void resized() override;

    /** Repaint when the audio thumbnail data changes */
    void changeListenerCallback (ChangeBroadcaster *source) override;

    /** Load an audio file to display its waveform */
    void loadURL(URL audioURL);

    /** Set the relative position of the playhead (0.0 to 1.0) */
    void setPositionRelative(double pos);

private:
    AudioThumbnail audioThumb;
    bool fileLoaded; 
    double position;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (WaveformDisplay)
};
