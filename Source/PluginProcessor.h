#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include "RecoveredEngine.h"
#include "FactoryPresets.h"
#include "ClearPreset.h"
#include "LegacyPresets.h"
class CamelPhatProcessor final:public juce::AudioProcessor {
public:
 CamelPhatProcessor();
 juce::AudioProcessorValueTreeState state;
 void prepareToPlay(double,int)override;void releaseResources()override{};
 void processBlock(juce::AudioBuffer<float>&,juce::MidiBuffer&)override;
 bool isBusesLayoutSupported(const BusesLayout&)const override;
 juce::AudioProcessorEditor* createEditor()override;bool hasEditor()const override{return true;}
 const juce::String getName()const override{return "CamelPhat Rebuilt";}
 bool acceptsMidi()const override{return true;}bool producesMidi()const override{return false;}double getTailLengthSeconds()const override{return .035;}
 int getNumPrograms()override{return int(hostProgramSlots.size());}int getCurrentProgram()override;
 void setCurrentProgram(int)override;const juce::String getProgramName(int)override;void changeProgramName(int,const juce::String&)override;
 void getStateInformation(juce::MemoryBlock&)override;void setStateInformation(const void*,int)override;
 float meter(int channel)const{return meters[size_t(channel)].load();}
 bool importLegacyBank(const juce::File&);
 bool saveLegacyPreset(const juce::File&);bool saveLegacyBank(const juce::File&);
 void setFactoryProgram(int);juce::String factoryPresetName(int)const;
 int currentPresetSlot()const{return currentProgram.load();}
 int presetSlotCount()const{return int(programs.size());}
 juce::String presetSlotName(int)const;void selectPresetSlot(int);
 bool isFactoryBank()const{return factoryMode;}
 void clearPreset();void randomize();
 void startMidiLearn(int id){learnTarget.store(id);}void cancelMidiLearn(){learnTarget.store(-1);}
 void clearMidiMapping(int id){if(id>=0&&id<53)midiCC[size_t(id)].store(-1);}
 int midiMappingForParameter(int id)const{return id>=0&&id<53?midiCC[size_t(id)].load():-1;}
private:
 static juce::AudioProcessorValueTreeState::ParameterLayout layout(CamelPhatProcessor*);
 std::array<std::atomic<float>*,53> values{};std::array<float,53> last{};
 camelphat_recovered::Engine engine;
 // Keep persisted bank slots separate from the compact list exposed to hosts.
 std::atomic<int> currentProgram{0};std::array<std::atomic<float>,2> meters{};
 std::array<std::atomic<int>,53> midiCC{};std::atomic<int> learnTarget{-1};std::atomic<bool> oldBitCrush{true};
 std::vector<camelphat::LegacyPreset> programs;juce::String customName;
 std::vector<int> hostProgramSlots;
 bool factoryMode=true;
 void storeCurrentProgram();void activateProgram(int);void rebuildHostPrograms();
 JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CamelPhatProcessor)
};
