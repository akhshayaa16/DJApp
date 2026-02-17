# Otodecks DJ Application — Report

**Module:** CM2005 Object Oriented Programming
**Author:** Chandrasekaran Akhshayaa

---

## Table of Contents

1. [R1: Basic Program](#r1-basic-program)
2. [R2: Music Library & Persistence](#r2-music-library--persistence)
3. [R3: Hot Cue Buttons & Persistence](#r3-hot-cue-buttons--persistence)
4. [R4: Three-Band Equaliser](#r4-three-band-equaliser)
5. [R5: Beats Per Minute (BPM)](#r5-beats-per-minute-bpm)
6. [Assumptions, Decisions & Unique Features](#assumptions-decisions--unique-features)

---

## R1: Basic Program

### R1A: Load Audio Files into Audio Players

The `DJAudioPlayer::loadURL()` method handles file loading. It creates an `InputStream` from the provided URL and passes it to the `AudioFormatManager` to obtain an `AudioFormatReader`. The reader is wrapped in an `AudioFormatReaderSource` and connected to the `AudioTransportSource` for playback.

```cpp
void DJAudioPlayer::loadURL(URL audioURL)
{
    auto stream = audioURL.createInputStream(false);
    auto* reader = formatManager.createReaderFor(std::move(stream));

    std::unique_ptr<AudioFormatReaderSource> newSource(
        new AudioFormatReaderSource(reader, true));

    transportSource.setSource(newSource.get(), 0, nullptr, reader->sampleRate);
    readerSource.reset(newSource.release());
}
```

Users can load files either through the LOAD button on each deck (which opens a file chooser dialog) or by dragging and dropping a file directly onto a deck. Files can also be loaded from the music library into a specific deck using the Deck 1 / Deck 2 buttons in the playlist table.

[Screenshot: A deck with a loaded track showing the waveform display]

### R1B: Play Two or More Tracks Simultaneously

The application uses two separate `DJAudioPlayer` instances (`player1` and `player2`), each connected to its own `DeckGUI`. Both players are added as input sources to a `MixerAudioSource` in `MainComponent`, which combines them into a single output stream:

```cpp
mixerSource.addInputSource(&player1, false);
mixerSource.addInputSource(&player2, false);
```

The `getNextAudioBlock()` method in `MainComponent` simply delegates to the mixer, which internally fetches audio from both players and sums them together:

```cpp
void MainComponent::getNextAudioBlock(const AudioSourceChannelInfo& bufferToFill)
{
    mixerSource.getNextAudioBlock(bufferToFill);
}
```

This allows both decks to play audio independently and simultaneously.

[Screenshot: Both decks playing tracks at the same time, with PLAY button highlighted on each]

### R1C: Mix Tracks by Varying Each of Their Volumes

Each deck has a volume slider (`volSlider`) with a range of 0.0 to 1.0. When the slider value changes, `DJAudioPlayer::setGain()` is called, which applies the gain to the `AudioTransportSource`:

```cpp
void DJAudioPlayer::setGain(double gain)
{
    gain = juce::jlimit(0.0, 1.0, gain);
    transportSource.setGain(gain);
}
```

Since each deck has its own independent volume control and they are mixed through the `MixerAudioSource`, the DJ can adjust the volume of each track independently to create smooth transitions and mixes.

[Screenshot: Two decks with different volume slider positions]

### R1D: Speed Up and Slow Down the Tracks

Each deck has a speed slider (`speedSlider`) with a range of 0.5 to 2.0 (half speed to double speed). The `DJAudioPlayer::setSpeed()` method adjusts the resampling ratio of the `ResamplingAudioSource`:

```cpp
void DJAudioPlayer::setSpeed(double ratio)
{
    ratio = juce::jlimit(0.1, 4.0, ratio);
    resampleSource.setResamplingRatio(ratio);
}
```

The `ResamplingAudioSource` wraps the `AudioTransportSource` and resamples the audio in real time, effectively changing the playback speed. A ratio of 1.0 means normal speed, values below 1.0 slow down the track, and values above 1.0 speed it up.

[Screenshot: A deck with the speed slider adjusted away from the default 1.0 position]

---

## R2: Music Library & Persistence

### R2A: Load Multiple Files and Display Track Details

The `PlaylistComponent` class implements a music library using a `TableListBox` with four columns: Track Title, Duration, Deck 1, and Deck 2. The "ADD TRACKS" button opens a file chooser that supports multi-file selection:

```cpp
auto flags = FileBrowserComponent::canSelectFiles
           | FileBrowserComponent::canSelectMultipleItems;

fChooser.launchAsync(flags, [this](const FileChooser& fc)
{
    auto results = fc.getResults();
    for (auto file : results)
    {
        TrackInfo t;
        t.filePath = file.getFullPathName();
        t.fileName = file.getFileName();
        t.durationSec = getTrackDurationSec(file);
        tracks.push_back(t);
    }
    tableComponent.updateContent();
    saveLibrary();
});
```

Track duration is calculated at import time by reading the audio file header through the `AudioFormatReader`:

```cpp
double PlaylistComponent::getTrackDurationSec(File file)
{
    std::unique_ptr<AudioFormatReader> reader(
        formatManager.createReaderFor(std::move(inputStream)));
    return (double)reader->lengthInSamples / reader->sampleRate;
}
```

The duration is displayed in minutes:seconds format (e.g., "3:42") in the table.

[Screenshot: Playlist table showing multiple tracks with filenames and durations]

### R2B: Load Music from the Library to Different Decks

Each row in the playlist table has two buttons: "Load" under the Deck 1 column and "Load" under the Deck 2 column. These are created dynamically in `refreshComponentForCell()` and identified using component IDs (e.g., `"deck1_0"`, `"deck2_3"`).

When clicked, the `buttonClicked()` handler parses the component ID to determine which deck and which row was selected, then calls the appropriate callback:

```cpp
if (id.startsWith("deck1_"))
{
    int row = id.fromFirstOccurrenceOf("deck1_", false, false).getIntValue();
    if (row >= 0 && row < (int)tracks.size() && loadToDeck1 != nullptr)
        loadToDeck1(File{tracks[row].filePath});
}
```

The callbacks `loadToDeck1` and `loadToDeck2` are set by `MainComponent` to call `deckGUI1.loadFile()` and `deckGUI2.loadFile()` respectively.

[Screenshot: Playlist with "Load" buttons visible for Deck 1 and Deck 2 columns]

### R2C: Music Library State Persists After Re-opening

The library is saved as a JSON array to `library.json` located in the user's application data directory under an `Otodecks` folder. Each track is stored as a JSON object containing `filePath`, `fileName`, and `durationSec`.

```cpp
void PlaylistComponent::saveLibrary()
{
    Array<var> arr;
    for (auto& t : tracks)
    {
        DynamicObject::Ptr obj = new DynamicObject();
        obj->setProperty("filePath", t.filePath);
        obj->setProperty("fileName", t.fileName);
        obj->setProperty("durationSec", t.durationSec);
        arr.add(var(obj.get()));
    }
    File file = getLibraryFile();
    file.replaceWithText(JSON::toString(var(arr)));
}
```

The library is automatically loaded in the constructor via `loadLibrary()` and saved in the destructor via `saveLibrary()`. It is also saved immediately after adding new tracks so that the library is not lost if the application closes unexpectedly.

**Decision:** JSON was chosen as the storage format for its human-readability and ease of debugging. The `userApplicationDataDirectory` location ensures the data is stored in a platform-appropriate location.

[Screenshot: Application re-opened showing previously loaded tracks still in the playlist]

---

## R3: Hot Cue Buttons & Persistence

### R3A: Assign Hot Cues While Playing or Paused

Each deck has 8 hot cue buttons and a "CUE MODE" toggle. When CUE MODE is enabled (toggled on), clicking a hot cue button assigns the current playback position to that cue:

```cpp
if (cueModeButton.getToggleState())
{
    hotCues[i] = player->getPositionRelative();
    saveHotCuesForCurrentTrack();
}
```

Hot cue positions are stored as relative values between 0.0 and 1.0 (fraction of total track length). A value of -1.0 indicates an empty (unassigned) cue slot. When CUE MODE is off, clicking an assigned cue jumps the playback to that position:

```cpp
else
{
    if (hotCues[i] >= 0.0)
        player->setPositionRelative(hotCues[i]);
}
```

This works regardless of whether the track is playing or paused, allowing the DJ to set cue points during preparation or live performance.

[Screenshot: Deck with CUE MODE enabled, showing some assigned cues with checkmarks]

### R3B: Modify Cues Individually

To modify an existing cue, the user simply enables CUE MODE and clicks the cue button they wish to update. The current playback position overwrites the previous value stored for that cue slot. Each cue is independent — modifying one does not affect the others. The change is immediately persisted to disk.

### R3C: Clear All Hot Cues

The "CLEAR CUES" button resets all 8 hot cue positions to -1.0 (empty) and updates the button labels to remove the checkmark indicators:

```cpp
void DeckGUI::clearAllHotCues()
{
    for (auto& c : hotCues) c = -1.0;
    updateHotCueButtonLabels();
}
```

After clearing, the state is saved to disk so the cleared state persists.

[Screenshot: Deck after clearing all cues, buttons showing "CUE 1" through "CUE 8" without checkmarks]

### R3D: Persist and Load Hot Cues Per Track

Hot cues are stored in `hotcues.json` in the `Otodecks` application data directory. The JSON structure uses the track's full file path as the key, mapping to an array of 8 double values:

```cpp
void DeckGUI::saveHotCuesForCurrentTrack()
{
    if (loadedTrackPath.isEmpty()) return;

    var root = loadHotCuesJson();
    if (!root.isObject())
        root = var(new DynamicObject());

    auto* obj = root.getDynamicObject();

    Array<var> arr;
    for (int i = 0; i < 8; ++i)
        arr.add(var(hotCues[(size_t)i]));

    obj->setProperty(loadedTrackPath, var(arr));
    saveHotCuesJson(root);
}
```

When a track is loaded into a deck, `loadHotCuesForCurrentTrack()` reads the JSON file and restores the cue positions for that specific track. This means each track remembers its own set of hot cues across application sessions.

**Decision:** Using the full file path as the key ensures that tracks with the same filename in different directories have separate cue data. The trade-off is that moving a file to a different location will lose its cue associations.

[Screenshot: Application re-opened with a previously loaded track showing its persisted cue assignments]

---

## R4: Three-Band Equaliser

### R4A: Three Knobs for High, Mid and Low EQ

Each deck features three rotary knobs labelled LOW, MID, and HIGH, allowing the DJ to adjust the equaliser in real time. The knobs have a range of -12 dB to +12 dB with 0 dB as the default (flat response). Double-clicking a knob resets it to 0 dB.

The EQ is implemented using JUCE's DSP module with a `ProcessorChain` containing three IIR filters:

- **Low band:** Low shelf filter at 200 Hz
- **Mid band:** Peak (bell) filter at 1000 Hz
- **High band:** High shelf filter at 6000 Hz

```cpp
auto low  = Coeffs::makeLowShelf(currentSampleRate, lowFreqHz, midQ,
                                  juce::Decibels::decibelsToGain(lowGainDb));
auto mid  = Coeffs::makePeakFilter(currentSampleRate, midFreqHz, midQ,
                                  juce::Decibels::decibelsToGain(midGainDb));
auto high = Coeffs::makeHighShelf(currentSampleRate, highFreqHz, midQ,
                                  juce::Decibels::decibelsToGain(highGainDb));
```

The audio processing applies the EQ chain per channel (left and right separately) to maintain correct stereo behaviour and avoid JUCE's mono-block assertion:

```cpp
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
```

**Decision:** Separate left and right `ProcessorChain` instances are used rather than a single stereo chain. This is because the JUCE `IIR::Filter` processes one channel at a time, and passing a stereo block to a mono-configured filter causes an assertion failure. Processing each channel independently solves this while maintaining correct filter state for each channel.

[Screenshot: EQ panel showing three rotary knobs with adjusted values, colour-coded (amber for LOW, grey for MID, red for HIGH)]

### R4B: Persist EQ Settings Per Track

EQ settings are stored in `eq.json` in the `Otodecks` application data directory. For each track (keyed by file path), both the last-adjusted values and the original baseline values are saved:

```cpp
DynamicObject::Ptr eq = new DynamicObject();
eq->setProperty("low",  lowDb);
eq->setProperty("mid",  midDb);
eq->setProperty("high", highDb);
eq->setProperty("originalLow",  originalLowDb);
eq->setProperty("originalMid",  originalMidDb);
eq->setProperty("originalHigh", originalHighDb);
```

When a track is loaded, `loadEQForCurrentTrack()` restores the saved EQ values, updates the slider positions, and applies the settings to the audio player. If no saved data exists for a track, all values default to 0 dB (flat response).

EQ settings are saved immediately whenever a slider changes and also in the destructor when the deck is destroyed, ensuring no data is lost.

**Decision:** Storing both the "original" and "last adjusted" values fulfils the requirement of persisting both pieces of information. The original values represent the baseline EQ (always 0 dB for a new track), while the last-adjusted values capture the DJ's custom settings.

[Screenshot: Application re-opened with a track showing its previously saved EQ settings restored on the knobs]

---

## R5: Beats Per Minute (BPM)

### R5A: What is BPM and How It Is Used in DJ Applications

**Beats Per Minute (BPM)** is a measure of the tempo of a music track — it counts how many beats occur in one minute. In DJ applications, BPM is a fundamental piece of metadata that drives several core workflows.

**Scenario 1: Beatmatching and Tempo Synchronisation**

Beatmatching is the process of adjusting the speed of one track so its BPM matches another track that is already playing. This ensures the beats of both tracks align, enabling the DJ to create a seamless transition. For example, if Track A is playing at 128 BPM and Track B is at 125 BPM, the DJ would increase the speed of Track B by approximately 2.4% to match. In the Otodecks application, the DJ can use the displayed BPM values on each deck along with the speed slider to manually match tempos.

**Scenario 2: Set Planning and Track Organisation**

DJs use BPM to organise their music library and plan the flow of a set. Tracks are typically arranged in order of increasing or gradually changing BPM to build energy on the dance floor. For instance, a DJ might start a set with tracks around 100 BPM (deep house) and progressively move to 128 BPM (tech house) and beyond. Knowing the BPM of each track allows the DJ to select compatible tracks for mixing and avoid jarring tempo jumps.

### R5B: Alternative Techniques

Several alternative techniques and features are available in popular DJ applications that serve similar purposes to manual BPM-based beatmatching:

1. **Auto-Sync (Traktor, rekordbox, Serato):** The software automatically adjusts the playback speed of one track to match the BPM of another, and aligns the beat grids so the downbeats coincide. This eliminates the need for manual speed adjustment.

2. **Key Detection and Harmonic Mixing (Mixed In Key, rekordbox):** Beyond tempo, these tools analyse the musical key of each track. DJs can then mix tracks that are in compatible keys, resulting in harmonically pleasing transitions even if the tempos differ slightly.

3. **Waveform Visual Alignment:** Modern DJ software displays detailed waveforms with colour-coded frequency information. DJs can visually align the transient peaks (kick drums, snares) of two tracks rather than relying solely on numerical BPM values, which is useful when beat grids are slightly inaccurate.

### R5C: How BPM Is Calculated

The BPM calculation follows a signal processing pipeline:

1. **Mono Conversion:** The stereo audio is averaged into a single mono channel to simplify analysis without losing rhythmic information.

2. **Energy Envelope Extraction:** The mono signal is divided into short overlapping windows (1024 samples with a 512-sample hop). For each window, the RMS (root mean square) energy is calculated, producing an "energy over time" envelope that captures the rhythmic pulse of the music.

3. **Normalisation and Smoothing:** The energy envelope is normalised to a 0–1 range and smoothed using a moving average filter to reduce noise while preserving the beat pattern.

4. **Mean Removal and Rectification:** The mean is subtracted from the smoothed envelope, and negative values are set to zero (half-wave rectification). This focuses the signal on the energy peaks that correspond to beats.

5. **Autocorrelation:** The processed envelope is correlated with time-shifted versions of itself across a range of lags corresponding to the expected BPM range (70–200 BPM). The lag with the highest autocorrelation score indicates the most likely beat period.

6. **BPM Conversion:** The winning lag (in envelope frames) is converted to BPM using the formula: `BPM = 60 * envelopeRate / lag`. A half/double correction ensures the result falls within the expected range (since autocorrelation can lock onto half-time or double-time patterns).

### R5D: Implementation and Visualisation

The BPM detection is implemented in the `BPMDetector` class with a single static method:

```cpp
static double detectBpmFromBuffer(const juce::AudioBuffer<float>& buffer,
                                  double sampleRate,
                                  double minBpm = 70.0,
                                  double maxBpm = 200.0);
```

The detection is triggered in `DJAudioPlayer::loadURL()` immediately after loading a track. Up to the first 60 seconds of audio are read into an analysis buffer and passed to the detector:

```cpp
const int maxSecondsToRead = 60;
const int maxSamplesToRead = (int) std::min<int64>(
    reader->lengthInSamples, (int64)(sr * maxSecondsToRead));

juce::AudioBuffer<float> analysisBuffer(numCh, maxSamplesToRead);
reader->read(&analysisBuffer, 0, maxSamplesToRead, 0, true, true);

bpm = BPMDetector::detectBpmFromBuffer(analysisBuffer, sr, 70.0, 200.0);
```

The detected BPM is visualised in the `DeckGUI` via a label positioned between the transport controls and the sliders. The label is updated both on track load and periodically via the timer callback:

```cpp
void DeckGUI::updateBpmLabel()
{
    const double b = player->getBpm();
    bpmLabel.setText(b > 0.0 ? ("BPM: " + String(b, 1)) : "BPM: --",
                     dontSendNotification);
}
```

The BPM is displayed with one decimal place (e.g., "BPM: 127.8"). If the detector cannot confidently estimate the BPM, it returns 0.0 and the label shows "BPM: --".

**Decision:** Analysing only the first 60 seconds of audio is a deliberate trade-off between accuracy and performance. Most tracks establish their tempo within the first minute, and limiting the analysis avoids excessive memory usage and processing time for long tracks.

[Screenshot: Two decks with loaded tracks, each showing their detected BPM value (e.g., "BPM: 127.8" and "BPM: 140.0")]

---

## Assumptions, Decisions & Unique Features

### Assumptions

- **Audio Format Support:** The application supports whatever audio formats are registered through JUCE's `AudioFormatManager::registerBasicFormats()`, which typically includes WAV, AIFF, and platform-dependent formats (e.g., MP3 on macOS/Windows via system codecs).
- **File Paths as Identifiers:** Track file paths are used as unique identifiers for hot cue and EQ persistence. This assumes tracks are not moved or renamed between sessions.
- **Sample Rate Consistency:** The EQ filter coefficients are calculated based on the system's sample rate at initialisation time. If the system sample rate changes during playback, the filters may not respond accurately until the next `prepareToPlay()` call.

### Key Design Decisions

- **JSON for Persistence:** All persistent data (library, hot cues, EQ) is stored as JSON files in the platform's application data directory. JSON was chosen for its readability, ease of debugging, and native support in JUCE via `JSON::parse()` and `JSON::toString()`.
- **Per-Channel EQ Processing:** Separate left and right filter chains are used to avoid JUCE's mono assertion while maintaining correct stereo filter state.
- **Immediate Persistence:** Changes to hot cues and EQ are saved to disk immediately (not just on application exit), preventing data loss from unexpected closures.
- **BPM Analysis Limit:** Only the first 60 seconds of audio are analysed for BPM to balance accuracy with performance.

### Unique Features

- **Dark Professional Theme:** The application uses a custom dark colour scheme with a dark background, rounded panels, and accent colours, giving it a modern DJ software aesthetic.
- **Visual Cue Feedback:** Hot cue buttons show a checkmark when assigned and flash with colour feedback when triggered (cyan) or assigned (green), providing clear visual indication of cue state.
- **Colour-Coded EQ Knobs:** The LOW knob is amber, MID is grey, and HIGH is red, matching common DJ mixer conventions and making it easy to identify each band at a glance.
- **Real-Time BPM Display:** BPM is calculated automatically on track load and displayed on each deck, aiding the DJ in beatmatching without needing external tools.
- **Drag and Drop:** In addition to the file chooser, tracks can be loaded into a deck by dragging and dropping audio files directly onto the deck panel.

---

*Note: Replace all `[Screenshot: ...]` placeholders with actual screenshots of the running application before converting this report to PDF for submission.*
