#pragma once
#include <array>
#include <limits>
namespace camelphat {
// IDs, raw type tags, endpoints from original x86_64 constructor.
#if 0
Defaults are unknown: constructor endpoints are NOT preset defaults.
#endif
struct ParameterSpec { int id; const char* name; const char* shortName; const char* unit; int originalType; float minimum; float maximum; bool smoothed; };
inline constexpr int kHostParameterCount = 53;
inline constexpr int kInternalParameterCount = 66;
inline constexpr int kEditorWidth = 570, kEditorHeight = 500;
inline constexpr std::array<ParameterSpec, 66> kParameterSpecs {{
    {0, "BpFilterOn", "BpFilterOn", "", 0, 0.0f, 1.0f, false},
    {1, "BpFilterLow", "BpFilterLow", "Hz", 5, 0.0f, 1.0f, true},
    {2, "BpFilterHigh", "BpFilterHigh", "Hz", 5, 0.0f, 1.0f, true},
    {3, "BpFilterQLow", "BpFilterQLow", "%", 1, 0.0f, 1.0f, true},
    {4, "BpFilterQHigh", "BpFilterQHigh", "%", 1, 0.0f, 1.0f, true},
    {5, "BpFilterBRMix", "BpFilterBRMix", "%", 1, 0.0f, 1.0f, true},
    {6, "BpFilterLink", "BpFilterLink", "", 0, 0.0f, 1.0f, false},
    {7, "DistOn", "DistOn", "", 0, 0.0f, 1.0f, false},
    {8, "DistBitCrusher", "DistBitCrusher", "Bit", 10, 0.0f, 1.0f, true},
    {9, "DistMech", "DistMech", "%", 1, 0.0f, 1.0f, true},
    {10, "DistTube", "DistTube", "%", 1, 0.0f, 1.0f, true},
    {11, "DistXcita", "DistXcita", "%", 1, 0.0f, 1.0f, true},
    {12, "MmFilterOn", "MmFilterOn", "", 0, 0.0f, 1.0f, false},
    {13, "MmFilterMix", "MmFilterMix", "%", 1, 0.0f, 1.0f, true},
    {14, "MmFilterType", "MmFilterType", "", 0, 0.0f, 100.0f, false},
    {15, "MmFilterAttack", "MmFilterAttack", "s", 4, 0.00999999978f, 1.0f, true},
    {16, "MmFilterDecay", "MmFilterDecay", "s", 4, 0.200000003f, 3.0f, true},
    {17, "MmFilterEnv", "MmFilterEnv", "%", 1, 0.0f, 1.0f, true},
    {18, "MmFilterCutoff", "MmFilterCutoff", "Hz", 6, 0.0f, 1.0f, true},
    {19, "MmFilterRes", "MmFilterRes", "%", 1, 0.0f, 1.0f, true},
    {20, "FlangerOn", "FlangerOn", "", 0, 0.0f, 1.0f, false},
    {21, "FlangerAmount", "FlangerAmnt", "%", 1, 0.0f, 1.0f, true},
    {22, "FlangerRate", "FlangerRate", "Hz", 4, 0.02f, 2.0f, true},
    {23, "MagicEQOn", "MagicEQOn", "", 0, 0.0f, 1.0f, false},
    {24, "MagicEQAmount", "MagicEQAmnt", "%", 1, 0.0f, 1.0f, true},
    {25, "MagicEQTune", "MagicEQTune", "Hz", 4, 60.0f, 120.0f, true},
    {26, "CompressOn", "CompressOn", "", 0, 0.0f, 1.0f, false},
    {27, "CompressAmount", "CompressAmnt", "dB", 7, 0.0f, 0.99000001f, true},
    {28, "CompressRelease", "CompressRel", "ms", 4, 100.0f, 2000.0f, true},
    {29, "MasterOn", "MasterOn", "", 0, 0.0f, 1.0f, false},
    {30, "MasterMix", "MasterMix", "%", 1, 0.0f, 1.0f, true},
    {31, "MasterVolume", "MasterVol", "dB", 8, 0.0f, 1.0f, true},
    {32, "Lfo1On", "Lfo1On", "", 0, 0.0f, 1.0f, false},
    {33, "Lfo1Target", "Lfo1Targ", "", 0, 0.0f, 100.0f, false},
    {34, "Lfo1Shape", "Lfo1Shape", "", 0, 0.0f, 100.0f, false},
    {35, "Lfo1Depth", "Lfo1Depth", "%", 1, 0.0f, 1.0f, true},
    {36, "Lfo1Rate", "Lfo1Rate", "", 2, 0.0f, 1.0f, true},
    {37, "Lfo1Sync", "Lfo1Sync", "", 0, 0.0f, 1.0f, false},
    {38, "Lfo2On", "Lfo2On", "", 0, 0.0f, 1.0f, false},
    {39, "Lfo2Target", "Lfo2Targ", "", 0, 0.0f, 100.0f, false},
    {40, "Lfo2Shape", "Lfo2Shape", "", 0, 0.0f, 100.0f, false},
    {41, "Lfo2Depth", "Lfo2Depth", "%", 1, 0.0f, 1.0f, true},
    {42, "Lfo2Rate", "Lfo2Rate", "", 2, 0.0f, 1.0f, true},
    {43, "Lfo2Sync", "Lfo2Sync", "", 0, 0.0f, 1.0f, false},
    {44, "MorphXParam", "XParam", "", 0, 0.0f, 100.0f, false},
    {45, "MorphYParam", "YParam", "", 0, 0.0f, 100.0f, false},
    {46, "CompressMode", "CompressMode", "", 0, 0.0f, 1.0f, false},
    {47, "MagicEQMode", "MagicEQMode", "", 0, 0.0f, 1.0f, false},
    {48, "Unused1", "Unused1", "", 0, 0.0f, 1.0f, false},
    {49, "Unused2", "Unused2", "", 0, 0.0f, 1.0f, false},
    {50, "Unused3", "Unused3", "", 0, 0.0f, 1.0f, false},
    {51, "Unused4", "Unused4", "", 0, 0.0f, 1.0f, false},
    {52, "Unused5", "Unused5", "", 0, 0.0f, 1.0f, false},
    {53, "Unused", "", "", 0, 0.0f, 1.0f, false},
    {54, "LfoSelect", "", "", 0, 0.0f, 1.0f, false},
    {55, "Randomize", "", "", 0, 0.0f, 1.0f, false},
    {56, "ParamDisplay", "", "", 0, 0.0f, 1.0f, false},
    {57, "MorphSquare", "", "", 0, 0.0f, 1.0f, false},
    {58, "LevelMeterL", "", "", 1, 0.0f, 1.0f, false},
    {59, "LevelMeterR", "", "", 1, 0.0f, 1.0f, false},
    {60, "BpFilterBar", "", "", 1, 0.0f, 1.0f, false},
    {61, "VersionDisplay", "", "", 1, 0.0f, 1.0f, false},
    {62, "GlobalMenu", "", "", 1, 0.0f, 1.0f, false},
    {63, "UpDownPreset", "", "", 1, 0.0f, 1.0f, false},
    {64, "Dummy", "", "", 1, 0.0f, 1.0f, false},
    {65, "InfoLink", "", "", 1, 0.0f, 1.0f, false},
}};
inline constexpr std::array<int, 27> kMorphTargets {{1, 2, 3, 4, 5, 8, 9, 10, 11, 13, 15, 16, 17, 18, 19, 21, 22, 24, 25, 27, 28, 30, 31, 36, 35, 42, 41}};
} // namespace camelphat
