#pragma once
#include <JuceHeader.h>
#include "ParameterSpec.h"
#include <map>
class CamelPhatEditor final : public juce::AudioProcessorEditor, private juce::Timer {
public:
    CamelPhatEditor(juce::AudioProcessor&, juce::AudioProcessorValueTreeState&);
    void paint(juce::Graphics&) override;
    void mouseDown(const juce::MouseEvent&) override;
    void mouseDoubleClick(const juce::MouseEvent&) override;
    void mouseDrag(const juce::MouseEvent&) override;
    void mouseUp(const juce::MouseEvent&) override;
    void mouseWheelMove(const juce::MouseEvent&, const juce::MouseWheelDetails&) override;
private:
    struct Control { juce::String name,type,image,page; int id=-1; juce::Rectangle<int> bounds; int x=0,y=0,x2=0,y2=0; };
    juce::AudioProcessorValueTreeState& state;
    std::vector<Control> controls;
    std::map<juce::String,juce::Image> images;
    juce::String glyphs;
    std::vector<int> glyphWidths;
    int selectedLfo=0, dragging=-1,pressedControl=-1,pressedFrame=0;
    float dragStart=0;
    bool metersEnabled=false,circular=false;
    std::array<float,2> meterValues{};
    double meterUpdated=0;
    juce::String displayGlyphs;
    std::vector<int> displayWidths;
    juce::String versionGlyphs;
    std::vector<int> versionWidths;
    std::unique_ptr<juce::FileChooser> chooser;
    juce::Point<float> dragOrigin;
    juce::String display;
    double displayUntil=0;
    void timerCallback() override;
    bool visible(const Control&) const;
    juce::RangedAudioParameter* parameter(int) const;
    float value(int) const;
    void setValue(int,float);
    void drawText(juce::Graphics&,const juce::String&,int,int);
    void selector(const Control&);
    void presetMenu(const Control&);
    void choosePresetFile(int operation);
    void knobMenu(const Control&);
    void drawVersion(juce::Graphics&);
    void drawDisplayText(juce::Graphics&,const juce::String&,int,int);
    void updateMorph(juce::Point<float>);
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CamelPhatEditor)
};
