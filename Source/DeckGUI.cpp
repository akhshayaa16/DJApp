/*
  ==============================================================================

    DeckGUI.cpp
    Author:  Chandrasekaran Akhshayaa

  ==============================================================================
*/

#include "DeckGUI.h"

using namespace juce;

//==============================================================================
// Small UI helpers (file-local)
//==============================================================================

static void styleBandLabel(Label& l, const String& text)
{
    l.setText(text, dontSendNotification);
    l.setJustificationType(Justification::centred);
    l.setFont(juce::FontOptions(12.0f).withStyle("Bold"));
    l.setColour(Label::textColourId, Colours::white.withAlpha(0.88f));
}

static void styleValueTextBox(Slider& s)
{
    s.setColour(Slider::textBoxTextColourId, Colours::white.withAlpha(0.92f));
    s.setColour(Slider::textBoxBackgroundColourId, Colours::black.withAlpha(0.22f));
    s.setColour(Slider::textBoxOutlineColourId, Colours::white.withAlpha(0.18f));
}

static void styleButton(TextButton& b, Colour base, Colour text = Colours::white)
{
    b.setColour(TextButton::buttonColourId, base);
    b.setColour(TextButton::textColourOffId, text.withAlpha(0.95f));
}

static void styleRotaryKnob(Slider& s, Colour fill, Colour outline = Colours::white)
{
    s.setColour(Slider::rotarySliderFillColourId, fill.withAlpha(0.92f));
    s.setColour(Slider::rotarySliderOutlineColourId, outline.withAlpha(0.18f));
    s.setColour(Slider::thumbColourId, fill);
    s.setColour(Slider::textBoxOutlineColourId, fill.withAlpha(0.28f));
}

//==============================================================================

DeckGUI::DeckGUI(DJAudioPlayer* _player,
                 AudioFormatManager& formatManagerToUse,
                 AudioThumbnailCache& cacheToUse)
    : waveformDisplay(formatManagerToUse, cacheToUse),
      player(_player)
{
    addAndMakeVisible(playButton);
    addAndMakeVisible(stopButton);
    addAndMakeVisible(loadButton);

    addAndMakeVisible(volSlider);
    addAndMakeVisible(speedSlider);
    addAndMakeVisible(posSlider);

    addAndMakeVisible(waveformDisplay);

    addAndMakeVisible(lowEQSlider);
    addAndMakeVisible(midEQSlider);
    addAndMakeVisible(highEQSlider);

    addAndMakeVisible(lowEQLabel);
    addAndMakeVisible(midEQLabel);
    addAndMakeVisible(highEQLabel);

    addAndMakeVisible(cueModeButton);
    addAndMakeVisible(clearCuesButton);

    // ✅ BPM label
    addAndMakeVisible(bpmLabel);
    bpmLabel.setJustificationType(Justification::centredRight);
    bpmLabel.setFont(juce::FontOptions(12.0f));
    bpmLabel.setColour(Label::textColourId, Colours::white.withAlpha(0.85f));
    bpmLabel.setText("BPM: --", dontSendNotification);

    for (auto& btn : hotCueButtons)
    {
        addAndMakeVisible(btn);
        btn.addListener(this);
    }

    playButton.addListener(this);
    stopButton.addListener(this);
    loadButton.addListener(this);

    clearCuesButton.addListener(this);
    cueModeButton.addListener(this);

    volSlider.addListener(this);
    speedSlider.addListener(this);
    posSlider.addListener(this);

    lowEQSlider.addListener(this);
    midEQSlider.addListener(this);
    highEQSlider.addListener(this);

    volSlider.setRange(0.0, 1.0);
    speedSlider.setRange(0.5, 2.0);
    speedSlider.setValue(1.0);
    posSlider.setRange(0.0, 1.0);

    styleValueTextBox(volSlider);
    styleValueTextBox(speedSlider);
    styleValueTextBox(posSlider);

    auto setupEQ = [](Slider& s)
    {
        s.setSliderStyle(Slider::RotaryHorizontalVerticalDrag);
        s.setTextBoxStyle(Slider::TextBoxBelow, false, 64, 18);
        s.setRange(-12.0, 12.0, 0.1);
        s.setValue(0.0);
        s.setDoubleClickReturnValue(true, 0.0);
    };

    setupEQ(lowEQSlider);
    setupEQ(midEQSlider);
    setupEQ(highEQSlider);

    styleValueTextBox(lowEQSlider);
    styleValueTextBox(midEQSlider);
    styleValueTextBox(highEQSlider);

    styleBandLabel(lowEQLabel, "LOW");
    styleBandLabel(midEQLabel, "MID");
    styleBandLabel(highEQLabel, "HIGH");

    const Colour btnBase   = Colour(0xff1f2937);
    const Colour btnAlt    = Colour(0xff0f172a);
    const Colour accent    = Colour(0xff00aaff);    // cyan
    const Colour lowCol    = Colour(0xffffb020);    // amber
    const Colour midCol    = Colour(0xffe5e7eb);    // grey
    const Colour highCol   = Colour(0xffef4444);    // red

    styleButton(playButton, btnBase);
    styleButton(stopButton, btnBase);
    styleButton(loadButton, btnBase);
    styleButton(clearCuesButton, btnAlt);

    clearCuesButton.setColour(TextButton::buttonOnColourId, accent.withAlpha(0.25f));
    cueModeButton.setColour(ToggleButton::textColourId, Colours::white.withAlpha(0.90f));

    for (auto& b : hotCueButtons)
    {
        b.setColour(TextButton::buttonColourId, Colour(0xff1b2635));
        b.setColour(TextButton::textColourOffId, Colours::white.withAlpha(0.92f));
        b.setColour(TextButton::buttonOnColourId, accent.withAlpha(0.30f));
    }

    styleRotaryKnob(lowEQSlider,  lowCol);
    styleRotaryKnob(midEQSlider,  midCol);
    styleRotaryKnob(highEQSlider, highCol);

    lowEQLabel.setColour(Label::textColourId,  lowCol.withAlpha(0.92f));
    midEQLabel.setColour(Label::textColourId,  Colours::white.withAlpha(0.88f));
    highEQLabel.setColour(Label::textColourId, highCol.withAlpha(0.92f));

    initHotCues();
    updateHotCueButtonLabels();

    startTimer(200);
}

DeckGUI::~DeckGUI()
{
    saveHotCuesForCurrentTrack();
    saveEQForCurrentTrack();
    stopTimer();
}

//==============================================================================

void DeckGUI::paint(Graphics& g)
{
    const Colour bg      = Colour(30, 35, 40);
    const Colour panel   = Colour(40, 45, 50);
    const Colour border  = Colour(70, 75, 82);
    const Colour accent  = Colour(0, 170, 255);

    g.fillAll(bg);

    auto panelArea = getLocalBounds().reduced(6);
    g.setColour(panel);
    g.fillRoundedRectangle(panelArea.toFloat(), 10.0f);

    g.setColour(border.withAlpha(0.85f));
    g.drawRoundedRectangle(panelArea.toFloat(), 10.0f, 1.25f);

    auto topStrip = panelArea.removeFromTop(3);
    g.setColour(accent.withAlpha(0.12f));
    g.fillRect(topStrip);

    if (!loadedTrackPath.isEmpty())
    {
        g.setColour(accent.withAlpha(0.06f));
        g.fillRoundedRectangle(getLocalBounds().reduced(10).toFloat(), 10.0f);
    }
}

void DeckGUI::resized()
{
    const int padding = 12;
    auto area = getLocalBounds().reduced(padding);

    const int gap = 10;
    const int smallGap = 6;
    const int controlH = 34;

    const int cueTopH     = controlH;
    const int cuesGridH   = 80;
    const int loadH       = controlH;

    const int transportH  = controlH * 2 + smallGap;
    const int slidersH    = controlH * 3 + smallGap*2;

    int eqH       = 120;
    int waveformH = 60;

    const int requiredFixed =
        transportH + gap +
        slidersH   + gap +
        cueTopH    + smallGap +
        cuesGridH  + gap +
        loadH;

    int remaining = area.getHeight() - requiredFixed;

    if (remaining < 0)
    {
        int shortBy = -remaining;

        const int minEqH = 80;
        const int minWaveH = 30;

        int eqReducible = eqH - minEqH;
        int waveReducible = waveformH - minWaveH;

        int takeFromEq = jmin(eqReducible, shortBy);
        eqH -= takeFromEq;
        shortBy -= takeFromEq;

        int takeFromWave = jmin(waveReducible, shortBy);
        waveformH -= takeFromWave;
        shortBy -= takeFromWave;
    }

    // Transport
    playButton.setBounds(area.removeFromTop(controlH));
    area.removeFromTop(smallGap);
    stopButton.setBounds(area.removeFromTop(controlH));

    // ✅ BPM label row (always visible)
    auto bpmRow = area.removeFromTop(18);
    bpmLabel.setBounds(bpmRow.removeFromRight(140));

    area.removeFromTop(gap);

    // Sliders
    volSlider.setBounds(area.removeFromTop(controlH));
    area.removeFromTop(smallGap);
    speedSlider.setBounds(area.removeFromTop(controlH));
    area.removeFromTop(smallGap);
    posSlider.setBounds(area.removeFromTop(controlH));
    area.removeFromTop(gap);

    // EQ
    auto eqArea = area.removeFromTop(eqH);

    const int labelH = 18;
    const int colW = eqArea.getWidth() / 3;
    const int knobSize = jlimit(60, 100, eqArea.getHeight() - labelH - 10);

    auto layoutEQ = [&](int idx, Label& label, Slider& knob)
    {
        Rectangle<int> col(eqArea.getX() + idx * colW, eqArea.getY(), colW, eqArea.getHeight());

        label.setBounds(col.removeFromTop(labelH));
        label.setJustificationType(Justification::centred);

        col.removeFromTop(6);

        int kx = col.getX() + (col.getWidth() - knobSize) / 2;
        int ky = col.getY();
        knob.setBounds(kx, ky, knobSize, knobSize);
    };

    layoutEQ(0, lowEQLabel,  lowEQSlider);
    layoutEQ(1, midEQLabel,  midEQSlider);
    layoutEQ(2, highEQLabel, highEQSlider);

    area.removeFromTop(gap);

    // Cue top
    auto cueTop = area.removeFromTop(cueTopH);
    cueModeButton.setBounds(cueTop.removeFromLeft(cueTop.getWidth() / 2).reduced(4));
    clearCuesButton.setBounds(cueTop.reduced(4));

    area.removeFromTop(smallGap);

    // Hot cues grid
    auto cuesArea = area.removeFromTop(cuesGridH);

    const int btnW = cuesArea.getWidth() / 4;
    const int btnH = cuesArea.getHeight() / 2;

    for (int i = 0; i < 8; ++i)
    {
        int r = i / 4;
        int c = i % 4;

        hotCueButtons[i].setBounds(
            cuesArea.getX() + c * btnW,
            cuesArea.getY() + r * btnH,
            btnW - 6,
            btnH - 6
        );
    }

    area.removeFromTop(gap);

    // Waveform
    waveformDisplay.setBounds(area.removeFromTop(waveformH));
    area.removeFromTop(smallGap);

    // Load
    loadButton.setBounds(area.removeFromTop(loadH));
}

//==============================================================================

void DeckGUI::updateBpmLabel()
{
    if (player == nullptr)
    {
        bpmLabel.setText("BPM: —", dontSendNotification);
        return;
    }

    const double b = player->getBpm(); // make sure DJAudioPlayer has getBpm()
    bpmLabel.setText(b > 0.0 ? ("BPM: " + String(b, 1)) : "BPM: --", dontSendNotification);
}

void DeckGUI::loadFile(File file)
{
    if (!file.existsAsFile())
        return;

    loadedTrackPath = file.getFullPathName();

    player->loadURL(URL{ file });
    waveformDisplay.loadURL(URL{ file });

    loadHotCuesForCurrentTrack();
    loadEQForCurrentTrack();

    updateBpmLabel();   // ✅ update after loading

    repaint();
}

void DeckGUI::buttonClicked(Button* button)
{
    const Colour btnBase     = Colour(0xff1f2937);
    const Colour accent      = Colour(0, 170, 255);
    const Colour cueGlow     = Colour(0, 170, 255);
    const Colour cueAssign   = Colour(0xff22c55e);

    if (button == &playButton)
    {
        player->start();
        playButton.setColour(TextButton::buttonColourId, accent.withAlpha(0.42f));
        stopButton.setColour(TextButton::buttonColourId, btnBase);
        repaint();
        return;
    }

    if (button == &stopButton)
    {
        player->stop();
        playButton.setColour(TextButton::buttonColourId, btnBase);
        stopButton.setColour(TextButton::buttonColourId, btnBase);
        repaint();
        return;
    }

    if (button == &loadButton)
    {
        auto fileChooserFlags = FileBrowserComponent::canSelectFiles;

        fChooser.launchAsync(fileChooserFlags, [this](const FileChooser& chooser)
        {
            auto chosenFile = chooser.getResult();
            if (chosenFile.exists())
                loadFile(chosenFile);
        });
        return;
    }

    if (button == &clearCuesButton)
    {
        clearAllHotCues();
        saveHotCuesForCurrentTrack();
        return;
    }

    for (int i = 0; i < 8; i++)
    {
        if (button == &hotCueButtons[i])
        {
            if (cueModeButton.getToggleState())
            {
                hotCues[i] = player->getPositionRelative();
                saveHotCuesForCurrentTrack();

                hotCueButtons[i].setColour(TextButton::buttonColourId, cueAssign.withAlpha(0.35f));
                hotCueButtons[i].setColour(TextButton::buttonOnColourId, cueAssign.withAlpha(0.45f));

                Timer::callAfterDelay(140, [safe = Component::SafePointer<DeckGUI>(this)]()
                {
                    if (safe != nullptr) safe->updateHotCueButtonLabels();
                });
            }
            else
            {
                if (hotCues[i] >= 0.0)
                {
                    player->setPositionRelative(hotCues[i]);

                    hotCueButtons[i].setColour(TextButton::buttonColourId, cueGlow.withAlpha(0.38f));
                    hotCueButtons[i].setColour(TextButton::buttonOnColourId, cueGlow.withAlpha(0.50f));

                    Timer::callAfterDelay(140, [safe = Component::SafePointer<DeckGUI>(this)]()
                    {
                        if (safe != nullptr) safe->updateHotCueButtonLabels();
                    });
                }
            }

            updateHotCueButtonLabels();
            return;
        }
    }
}

void DeckGUI::sliderValueChanged(Slider* slider)
{
    if (slider == &volSlider)   player->setGain(slider->getValue());
    if (slider == &speedSlider) player->setSpeed(slider->getValue());
    if (slider == &posSlider)   player->setPositionRelative(slider->getValue());

    if (slider == &lowEQSlider || slider == &midEQSlider || slider == &highEQSlider)
    {
        lowDb  = lowEQSlider.getValue();
        midDb  = midEQSlider.getValue();
        highDb = highEQSlider.getValue();

        applyEQToPlayer();
        saveEQForCurrentTrack();
    }
}

bool DeckGUI::isInterestedInFileDrag(const StringArray& files) { return files.size() == 1; }

void DeckGUI::filesDropped(const StringArray& files, int, int)
{
    if (files.size() == 1) loadFile(File{ files[0] });
}

void DeckGUI::timerCallback()
{
    const double pos = player->getPositionRelative();
    waveformDisplay.setPositionRelative(pos);
    posSlider.setValue(pos, dontSendNotification);

    updateBpmLabel(); // ✅ keeps BPM label correct even if you reload etc.
}

// ==========================
// HOT CUES IMPLEMENTATION
// ==========================

void DeckGUI::initHotCues()
{
    for (auto& c : hotCues) c = -1.0;
}

void DeckGUI::clearAllHotCues()
{
    for (auto& c : hotCues) c = -1.0;
    updateHotCueButtonLabels();
}

void DeckGUI::updateHotCueButtonLabels()
{
    for (int i = 0; i < 8; ++i)
    {
        if (hotCues[i] >= 0.0)
            hotCueButtons[i].setButtonText("CUE " + String(i + 1) + " ✓");
        else
            hotCueButtons[i].setButtonText("CUE " + String(i + 1));
    }
}

// ==========================
// HOT CUE PERSISTENCE (R3D)
// ==========================

File DeckGUI::getHotCuesFile()
{
    auto dir = File::getSpecialLocation(File::userApplicationDataDirectory)
                    .getChildFile("Otodecks");
    if (!dir.exists())
        dir.createDirectory();
    return dir.getChildFile("hotcues.json");
}

var DeckGUI::loadHotCuesJson()
{
    File f = getHotCuesFile();
    if (!f.existsAsFile()) return var();
    return JSON::parse(f.loadFileAsString());
}

void DeckGUI::saveHotCuesJson(var json)
{
    File f = getHotCuesFile();
    f.replaceWithText(JSON::toString(json));
}

void DeckGUI::loadHotCuesForCurrentTrack()
{
    initHotCues();

    if (loadedTrackPath.isEmpty()) return;

    var root = loadHotCuesJson();
    if (!root.isObject()) return;

    auto* obj = root.getDynamicObject();
    if (obj == nullptr) return;

    var trackData = obj->getProperty(loadedTrackPath);
    if (!trackData.isArray()) return;

    auto* arr = trackData.getArray();
    for (int i = 0; i < jmin(8, arr->size()); ++i)
        hotCues[(size_t)i] = (double)(*arr)[i];

    updateHotCueButtonLabels();
}

void DeckGUI::saveHotCuesForCurrentTrack()
{
    if (loadedTrackPath.isEmpty()) return;

    var root = loadHotCuesJson();
    if (!root.isObject())
        root = var(new DynamicObject());

    auto* obj = root.getDynamicObject();
    if (obj == nullptr) return;

    Array<var> arr;
    for (int i = 0; i < 8; ++i)
        arr.add(var(hotCues[(size_t)i]));

    obj->setProperty(loadedTrackPath, var(arr));
    saveHotCuesJson(root);
}

// ==========================
// EQ PERSISTENCE (R4B)
// ==========================

File DeckGUI::getEQFile()
{
    auto dir = File::getSpecialLocation(File::userApplicationDataDirectory)
                    .getChildFile("Otodecks");
    if (!dir.exists())
        dir.createDirectory();
    return dir.getChildFile("eq.json");
}

var DeckGUI::loadEQJson()
{
    File f = getEQFile();
    if (!f.existsAsFile()) return var();
    return JSON::parse(f.loadFileAsString());
}

void DeckGUI::saveEQJson(var json)
{
    File f = getEQFile();
    f.replaceWithText(JSON::toString(json));
}

void DeckGUI::applyEQToPlayer()
{
    if (!player) return;

    player->setLowEQGainDb((float) lowDb);
    player->setMidEQGainDb((float) midDb);
    player->setHighEQGainDb((float) highDb);
}

void DeckGUI::loadEQForCurrentTrack()
{
    lowDb  = 0.0;
    midDb  = 0.0;
    highDb = 0.0;

    if (loadedTrackPath.isEmpty()) return;

    var root = loadEQJson();
    if (!root.isObject()) return;

    auto* obj = root.getDynamicObject();
    if (obj == nullptr) return;

    var trackData = obj->getProperty(loadedTrackPath);
    if (!trackData.isObject()) return;

    auto* eq = trackData.getDynamicObject();
    if (eq == nullptr) return;

    lowDb  = (double) eq->getProperty("low");
    midDb  = (double) eq->getProperty("mid");
    highDb = (double) eq->getProperty("high");

    originalLowDb  = (double) eq->getProperty("originalLow");
    originalMidDb  = (double) eq->getProperty("originalMid");
    originalHighDb = (double) eq->getProperty("originalHigh");

    lowEQSlider.setValue(lowDb, dontSendNotification);
    midEQSlider.setValue(midDb, dontSendNotification);
    highEQSlider.setValue(highDb, dontSendNotification);

    applyEQToPlayer();
}

void DeckGUI::saveEQForCurrentTrack()
{
    if (loadedTrackPath.isEmpty()) return;

    var root = loadEQJson();
    if (!root.isObject())
        root = var(new DynamicObject());

    auto* obj = root.getDynamicObject();
    if (obj == nullptr) return;

    DynamicObject::Ptr eq = new DynamicObject();
    eq->setProperty("low",  lowDb);
    eq->setProperty("mid",  midDb);
    eq->setProperty("high", highDb);
    eq->setProperty("originalLow",  originalLowDb);
    eq->setProperty("originalMid",  originalMidDb);
    eq->setProperty("originalHigh", originalHighDb);

    obj->setProperty(loadedTrackPath, var(eq.get()));
    saveEQJson(root);
}


