#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "RecoveredRandomize.h"
#include "DefaultMidi.h"
#include <sstream>
namespace {
const char* filterNames[]={"LP2 BQ","LP2 Fat","BP2 BQ","BP2 Fat","HP2 BQ","HP2 Fat","Formant","Peaking","Notch","CombP","CombM","RingMod","Tube","Mech","Bit Red","LP2 SVF HQ","LP2 K20 HQ","LP2 XP HQ","LP2 MG HQ","LP4 XP HQ","LP4 MG HQ","BP2 SVF HQ","BP2 K20 HQ","BP2 XP HQ","BP2 MG HQ","BP4 XP HQ","BP4 MG HQ","HP2 SVF HQ","HP2 K20 HQ","HP2 XP HQ","HP2 MG HQ","HP4 XP HQ","HP4 MG HQ"};
const char* waves[]={"Sine","Triangle","Ramp Up","Ramp Down","Square","Random Square","Random Triangle","Inverted Sine","Inverted Triangle","Inverted Square"};
const char* rateNames[]={"1/32T","1/32","1/32*","1/16T","1/16","1/16*","1/8T","1/8","1/8*","1/4T","1/4","1/4*","1/2T","1/2","1/2*","1/1T","1/1","1/1*","2/1T","2/1","2/1*","4/1T","4/1","4/1*","8/1T","8/1","8/1*","16/1T","16/1","16/1*","32/1T","32/1","32/1*"};
juce::String text(int id,float v,bool sync){auto& s=camelphat::kParameterSpecs[size_t(id)];int selection=int(v*100.f+.5f);
 if(id==36||id==42){if(sync)return juce::String(rateNames[camelphat_recovered::lfoRateIndex(v)])+" bars";return juce::String(camelphat_recovered::lfoRateHz(v,false,120),1)+" Hz";}
 if(id==14)return filterNames[std::max(0,std::min(32,selection))];
 if(id==34||id==40)return waves[std::max(0,std::min(9,selection))];
 if(id==44||id==45||id==33||id==39){bool morph=id==44||id==45;int target=morph?camelphat::morphTargets[size_t(std::max(0,std::min(26,selection)))]:camelphat::modulationTargets[size_t(std::max(0,std::min(22,selection)))];return camelphat::kParameterSpecs[size_t(target)].shortName;}
 float raw=v*(s.maximum-s.minimum)+s.minimum;
 if(s.originalType==0)return v>.5f?"On":"Off";
 if(s.originalType==1)return juce::String(int(raw*100.f))+" %";
 if(s.originalType==5)return juce::String(int(camelphat::bandPassFrequencies[size_t(std::max(0,std::min(284,int(v*284.f))))]))+" Hz";
 if(s.originalType==6){float hz=camelphat_recovered::MMEngine::hertz(v,0,0)/camelphat_recovered::floatBits(0x3b14f209);return juce::String(int(hz))+" Hz";}
 if(s.originalType==10)return juce::String(int((1.f-v)*15.f+1.f))+" Bit";
 if(s.originalType==8)return juce::String(int(20.*std::log10(std::max(1e-10,std::pow(double(v),2.736999988555908)))))+" dB";
 if(s.originalType==7)return juce::String(int(20.f*std::log10(1.f-raw)))+" dB";
 return juce::String(raw,1)+(juce::String(s.unit).isEmpty()?"":" "+juce::String(s.unit));
}
}
juce::AudioProcessorValueTreeState::ParameterLayout CamelPhatProcessor::layout(CamelPhatProcessor* processor){juce::AudioProcessorValueTreeState::ParameterLayout result;
 for(int i=0;i<53;++i){auto& s=camelphat::kParameterSpecs[size_t(i)];auto attrs=juce::AudioParameterFloatAttributes().withStringFromValueFunction([processor,i](float v,int){bool sync=false;if(i==36||i==42){auto* value=processor->state.getRawParameterValue(i==36?"Lfo1Sync":"Lfo2Sync");sync=value&&value->load()>=.5f;}return text(i,v,sync);});
  result.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID(s.name,1),s.shortName,juce::NormalisableRange<float>(0,1),camelphat::factoryPresets[0].values[size_t(i)],attrs));}
 return result;
}
CamelPhatProcessor::CamelPhatProcessor():AudioProcessor(BusesProperties().withInput("Input",juce::AudioChannelSet::stereo(),true).withOutput("Output",juce::AudioChannelSet::stereo(),true)),state(*this,nullptr,"CamelPhatState",layout(this)){
 for(size_t i=0;i<53;++i){values[i]=state.getRawParameterValue(camelphat::kParameterSpecs[i].name);midiCC[i].store(camelphat::defaultMidiCC[i]);}
 for(auto& p:camelphat::factoryPresets)programs.push_back({p.name,p.values,p.oldBitCrush,p.bank});
 rebuildHostPrograms();
}
bool CamelPhatProcessor::isBusesLayoutSupported(const BusesLayout& b)const{return b.getMainInputChannelSet()==juce::AudioChannelSet::stereo()&&b.getMainOutputChannelSet()==juce::AudioChannelSet::stereo();}
void CamelPhatProcessor::prepareToPlay(double rate,int){for(size_t i=0;i<53;++i)last[i]=values[i]->load();engine.prepare(float(rate),last);engine.oldBitCrush=oldBitCrush.load();}
void CamelPhatProcessor::processBlock(juce::AudioBuffer<float>& buffer,juce::MidiBuffer& midi){juce::ScopedNoDenormals noDenormals;
 for(size_t i=0;i<53;++i){float v=values[i]->load();if(v!=last[i]){engine.set(int(i),v);last[i]=v;}}
 if(auto* play=getPlayHead())if(auto pos=play->getPosition())engine.time(float(pos->getBpm().orFallback(120.)),pos->getPpqPosition().orFallback(engine.beats),pos->getIsPlaying());
 const int n=buffer.getNumSamples();
 std::array<camelphat_recovered::Engine::ControlEvent,1024> events;int count=0;
 for(const auto metadata:midi){auto message=metadata.getMessage();if(!message.isController())continue;int cc=message.getControllerNumber();int learning=learnTarget.exchange(-1);if(learning>=0&&learning<53)midiCC[size_t(learning)].store(cc);int target=-1;for(int i=0;i<53;++i)if(midiCC[size_t(i)].load()==cc){target=i;break;}if(target>=0&&count<1024)events[size_t(count++)]={metadata.samplePosition,target,float(message.getControllerValue())/127.f};}
 engine.oldBitCrush=oldBitCrush.load();engine.process(buffer.getReadPointer(0),buffer.getReadPointer(1),buffer.getWritePointer(0),buffer.getWritePointer(1),n,events.data(),count);
 for(int i=0;i<53;++i)if(engine.normalized[size_t(i)]!=last[size_t(i)]){last[size_t(i)]=engine.normalized[size_t(i)];state.getParameter(camelphat::kParameterSpecs[size_t(i)].name)->setValueNotifyingHost(last[size_t(i)]);}
 for(size_t i=0;i<2;++i)meters[i].store(engine.meters[i]);midi.clear();
}
juce::AudioProcessorEditor* CamelPhatProcessor::createEditor(){return new CamelPhatEditor(*this,state);}
void CamelPhatProcessor::rebuildHostPrograms(){
 hostProgramSlots.clear();
 for(int i=0;i<presetSlotCount();++i){
  // An imported unnamed FXP or a cleared name must not remove an existing
  // factory program. Only the factory bank's unused padding is hidden.
  bool factoryNamed=factoryMode&&i<int(camelphat::factoryPresets.size())&&camelphat::factoryPresets[size_t(i)].name[0]!='\0';
  if(factoryNamed||juce::String(programs[size_t(i)].name).trim().isNotEmpty()||i==currentPresetSlot())hostProgramSlots.push_back(i);
 }
}
int CamelPhatProcessor::getCurrentProgram(){auto found=std::find(hostProgramSlots.begin(),hostProgramSlots.end(),currentPresetSlot());return found==hostProgramSlots.end()?0:int(found-hostProgramSlots.begin());}
void CamelPhatProcessor::storeCurrentProgram(){int index=currentPresetSlot();if(index<0||index>=presetSlotCount())return;auto& p=programs[size_t(index)];for(size_t i=0;i<53;++i)p.values[i]=values[i]->load();p.oldBitCrush=oldBitCrush.load();if(!customName.isEmpty())p.name=customName.toStdString();}
void CamelPhatProcessor::activateProgram(int index){currentProgram.store(index);customName.clear();rebuildHostPrograms();auto& p=programs[size_t(index)];oldBitCrush.store(p.oldBitCrush);for(size_t i=0;i<53;++i)state.getParameter(camelphat::kParameterSpecs[i].name)->setValueNotifyingHost(p.values[i]);}
void CamelPhatProcessor::selectPresetSlot(int index){if(index<0||index>=presetSlotCount())return;storeCurrentProgram();activateProgram(index);}
void CamelPhatProcessor::setCurrentProgram(int index){if(index<0||index>=getNumPrograms())return;selectPresetSlot(hostProgramSlots[size_t(index)]);}
void CamelPhatProcessor::setFactoryProgram(int index){if(index<0||index>=int(camelphat::factoryPresets.size()))return;if(factoryMode){selectPresetSlot(index);return;}programs.clear();for(auto& p:camelphat::factoryPresets)programs.push_back({p.name,p.values,p.oldBitCrush,p.bank});factoryMode=true;activateProgram(index);}
juce::String CamelPhatProcessor::factoryPresetName(int index)const{if(index<0||index>=int(camelphat::factoryPresets.size()))return {};return factoryMode&&index<int(programs.size())?juce::String(programs[size_t(index)].name):juce::String(camelphat::factoryPresets[size_t(index)].name);}
juce::String CamelPhatProcessor::presetSlotName(int i)const{if(i<0||i>=presetSlotCount())return {};if(i==currentPresetSlot()&&!customName.isEmpty())return customName;auto& p=programs[size_t(i)];auto name=juce::String(p.name);if(name.trim().isEmpty())name="blank";return p.bank.empty()?name:juce::String(p.bank)+" / "+name;}
const juce::String CamelPhatProcessor::getProgramName(int i){return i<0||i>=getNumPrograms()?juce::String():presetSlotName(hostProgramSlots[size_t(i)]);}
void CamelPhatProcessor::changeProgramName(int index,const juce::String& name){if(index<0||index>=getNumPrograms())return;int slot=hostProgramSlots[size_t(index)];programs[size_t(slot)].name=name.toStdString();if(slot==currentPresetSlot())customName=name;rebuildHostPrograms();}
void CamelPhatProcessor::getStateInformation(juce::MemoryBlock& block){storeCurrentProgram();auto copy=state.copyState();copy.setProperty("program",currentPresetSlot(),nullptr);copy.setProperty("factoryMode",factoryMode,nullptr);copy.setProperty("oldBitCrush",oldBitCrush.load(),nullptr);copy.setProperty("customName",customName,nullptr);for(int i=0;i<53;++i)copy.setProperty("cc"+juce::String(i),midiCC[size_t(i)].load(),nullptr);
 auto previous=copy.getChildWithName("Programs");if(previous.isValid())copy.removeChild(previous,nullptr);juce::ValueTree bank("Programs");for(auto& p:programs){juce::ValueTree item("Preset");item.setProperty("name",juce::String(p.name),nullptr);item.setProperty("bank",juce::String(p.bank),nullptr);item.setProperty("oldBitCrush",p.oldBitCrush,nullptr);for(int i=0;i<53;++i)item.setProperty("p"+juce::String(i),p.values[size_t(i)],nullptr);bank.addChild(item,-1,nullptr);}copy.addChild(bank,-1,nullptr);if(auto xml=copy.createXml())copyXmlToBinary(*xml,block);}
void CamelPhatProcessor::setStateInformation(const void* data,int size){if(auto xml=getXmlFromBinary(data,size))if(xml->hasTagName("CamelPhatState")){auto tree=juce::ValueTree::fromXml(*xml);auto bank=tree.getChildWithName("Programs");if(bank.isValid()&&bank.getNumChildren()>0&&bank.getNumChildren()<=1024){std::vector<camelphat::LegacyPreset> loaded;for(auto item:bank){camelphat::LegacyPreset p;p.name=item.getProperty("name").toString().toStdString();p.bank=item.getProperty("bank").toString().toStdString();p.oldBitCrush=bool(item.getProperty("oldBitCrush",false));for(int i=0;i<53;++i)p.values[size_t(i)]=juce::jlimit(0.f,1.f,float(item.getProperty("p"+juce::String(i),0.f)));loaded.push_back(std::move(p));}programs=std::move(loaded);}factoryMode=bool(tree.getProperty("factoryMode",programs.size()==camelphat::factoryPresets.size()));currentProgram.store(juce::jlimit(0,presetSlotCount()-1,int(tree.getProperty("program",0))));oldBitCrush.store(bool(tree.getProperty("oldBitCrush",false)));customName=tree.getProperty("customName");for(int i=0;i<53;++i)midiCC[size_t(i)].store(int(tree.getProperty("cc"+juce::String(i),camelphat::defaultMidiCC[size_t(i)])));state.replaceState(tree);rebuildHostPrograms();}}
bool CamelPhatProcessor::importLegacyBank(const juce::File& file){juce::MemoryBlock bytes;if(!file.loadFileAsData(bytes))return false;std::vector<camelphat::LegacyPreset> decoded;if(!camelphat::decodeLegacy(bytes.getData(),bytes.getSize(),decoded))return false;if(file.hasFileExtension("fxp")){if(decoded.size()!=1)return false;int index=currentPresetSlot();decoded[0].bank=programs[size_t(index)].bank;programs[size_t(index)]=std::move(decoded[0]);activateProgram(index);}else{for(auto& p:decoded)p.bank=file.getFileNameWithoutExtension().toStdString();programs=std::move(decoded);factoryMode=false;activateProgram(0);}return true;}
bool CamelPhatProcessor::saveLegacyPreset(const juce::File& file){storeCurrentProgram();auto bytes=camelphat::encodeLegacy({programs[size_t(currentPresetSlot())]},false);return !bytes.empty()&&file.replaceWithData(bytes.data(),bytes.size());}
bool CamelPhatProcessor::saveLegacyBank(const juce::File& file){storeCurrentProgram();size_t first=size_t(currentPresetSlot()/64)*64,last=std::min(first+64,programs.size());std::vector<camelphat::LegacyPreset> bank(programs.begin()+static_cast<std::ptrdiff_t>(first),programs.begin()+static_cast<std::ptrdiff_t>(last));auto bytes=camelphat::encodeLegacy(bank,true);return !bytes.empty()&&file.replaceWithData(bytes.data(),bytes.size());}
void CamelPhatProcessor::clearPreset(){oldBitCrush.store(false);customName="blank";for(size_t i=0;i<53;++i)state.getParameter(camelphat::kParameterSpecs[i].name)->setValueNotifyingHost(camelphat::clearPreset[i]);}
void CamelPhatProcessor::randomize(){std::array<float,53> p;for(size_t i=0;i<53;++i)p[i]=values[i]->load();camelphat_recovered::randomize(p);for(size_t i=0;i<53;++i)state.getParameter(camelphat::kParameterSpecs[i].name)->setValueNotifyingHost(p[i]);}
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter(){return new CamelPhatProcessor;}
