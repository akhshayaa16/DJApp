/*
  ==============================================================================

    PlaylistComponent.cpp
    Created: 4 Feb 2026 4:28:33pm
    Author:  Chandrasekaran Akhshayaa

  ==============================================================================
*/

#include <JuceHeader.h>
#include "PlaylistComponent.h"

//==============================================================================
PlaylistComponent::PlaylistComponent(AudioFormatManager& _formatManager)
: formatManager(_formatManager)
{
    tableComponent.getHeader().addColumn("Track title", 1, 400);
    tableComponent.getHeader().addColumn("Duration", 2, 150);
    tableComponent.getHeader().addColumn("Deck 1", 3, 120);
    tableComponent.getHeader().addColumn("Deck 2", 4, 120);

    tableComponent.setModel(this);

    addAndMakeVisible(addButton);
    addButton.addListener(this);

    addAndMakeVisible(tableComponent);

    loadLibrary();
}

PlaylistComponent::~PlaylistComponent()
{
    saveLibrary();
}

void PlaylistComponent::paint (juce::Graphics& g)
{
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));

    g.setColour (juce::Colours::grey);
    g.drawRect (getLocalBounds(), 1);

    g.setColour (juce::Colours::white);
    g.setFont (juce::FontOptions (14.0f));
    g.drawText ("PlaylistComponent", getLocalBounds(),
                juce::Justification::centred, true);
}

void PlaylistComponent::resized()
{
    auto area = getLocalBounds();
    auto top = area.removeFromTop(40);

    addButton.setBounds(top.removeFromLeft(160).reduced(5));
    tableComponent.setBounds(area);
}

int PlaylistComponent::getNumRows()
{
    return (int)tracks.size();
}

void PlaylistComponent::paintRowBackground(Graphics & g,
                                           int rowNumber,
                                           int width,
                                           int height,
                                           bool rowIsSelected)
{
    if (rowIsSelected)
    {
        g.fillAll(Colours::orange);
    }
    else
    {
        g.fillAll(Colours::darkgrey);
    }
}

void PlaylistComponent::paintCell(Graphics & g,
                                  int rowNumber,
                                  int columnId,
                                  int width,
                                  int height,
                                  bool rowIsSelected)
{
    if (rowNumber < 0 || rowNumber >= (int)tracks.size()) return;

    if (columnId == 1)
    {
        g.drawText(tracks[rowNumber].fileName,
                   2, 0,
                   width - 4, height,
                   Justification::centredLeft,
                   true);
    }

    if (columnId == 2)
    {
        int totalSeconds = (int)std::round(tracks[rowNumber].durationSec);
        int mins = totalSeconds / 60;
        int secs = totalSeconds % 60;

        String durationText = String(mins) + ":" + String(secs).paddedLeft('0', 2);

        g.drawText(durationText,
                   2, 0,
                   width - 4, height,
                   Justification::centredLeft,
                   true);
    }
}

Component* PlaylistComponent::refreshComponentForCell(int rowNumber,
                                                      int columnId,
                                                      bool isRowSelected,
                                                      Component *existingComponentToUpdate)
{
    if (columnId == 3 || columnId == 4)
    {
        if (existingComponentToUpdate == nullptr)
        {
            TextButton* btn = new TextButton{"Load"};
            btn->addListener(this);

            String id = (columnId == 3 ? "deck1_" : "deck2_") + String(rowNumber);
            btn->setComponentID(id);

            existingComponentToUpdate = btn;
        }
        else
        {
            String id = (columnId == 3 ? "deck1_" : "deck2_") + String(rowNumber);
            existingComponentToUpdate->setComponentID(id);
        }
    }

    return existingComponentToUpdate;
}

void PlaylistComponent::buttonClicked(Button * button)
{
    if (button == &addButton)
    {
        auto flags = FileBrowserComponent::canSelectFiles
                   | FileBrowserComponent::canSelectMultipleItems;

        fChooser.launchAsync(flags, [this](const FileChooser& fc)
        {
            auto results = fc.getResults();

            for (auto file : results)
            {
                if (file.existsAsFile())
                {
                    TrackInfo t;
                    t.filePath = file.getFullPathName();
                    t.fileName = file.getFileName();
                    t.durationSec = getTrackDurationSec(file);

                    tracks.push_back(t);
                }
            }

            tableComponent.updateContent();
            tableComponent.repaint();

            saveLibrary();
        });

        return;
    }

    String id = button->getComponentID();

    if (id.startsWith("deck1_"))
    {
        int row = id.fromFirstOccurrenceOf("deck1_", false, false).getIntValue();

        if (row >= 0 && row < (int)tracks.size() && loadToDeck1 != nullptr)
        {
            loadToDeck1(File{tracks[row].filePath});
        }

        return;
    }

    if (id.startsWith("deck2_"))
    {
        int row = id.fromFirstOccurrenceOf("deck2_", false, false).getIntValue();

        if (row >= 0 && row < (int)tracks.size() && loadToDeck2 != nullptr)
        {
            loadToDeck2(File{tracks[row].filePath});
        }

        return;
    }
}

double PlaylistComponent::getTrackDurationSec(File file)
{
    std::unique_ptr<InputStream> inputStream(file.createInputStream());
    if (inputStream == nullptr) return 0.0;

    std::unique_ptr<AudioFormatReader> reader(formatManager.createReaderFor(std::move(inputStream)));
    if (reader == nullptr) return 0.0;

    if (reader->sampleRate <= 0) return 0.0;

    return (double)reader->lengthInSamples / reader->sampleRate;
}

File PlaylistComponent::getLibraryFile()
{
    auto dir = File::getSpecialLocation(File::userApplicationDataDirectory)
                    .getChildFile("Otodecks");

    if (!dir.exists())
    {
        dir.createDirectory();
    }

    return dir.getChildFile("library.json");
}

void PlaylistComponent::loadLibrary()
{
    File file = getLibraryFile();
    if (!file.existsAsFile()) return;

    auto jsonText = file.loadFileAsString();
    auto parsed = JSON::parse(jsonText);

    if (!parsed.isArray()) return;

    auto* arr = parsed.getArray();
    if (arr == nullptr) return;

    tracks.clear();

    for (auto& item : *arr)
    {
        if (!item.isObject()) continue;

        auto* obj = item.getDynamicObject();
        if (obj == nullptr) continue;

        TrackInfo t;
        t.filePath = obj->getProperty("filePath").toString();
        t.fileName = obj->getProperty("fileName").toString();
        t.durationSec = (double)obj->getProperty("durationSec");

        tracks.push_back(t);
    }

    tableComponent.updateContent();
    tableComponent.repaint();
}

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
