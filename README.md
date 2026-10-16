# CamelPhat Rebuilt

Native Apple Silicon AU, VST3, and standalone builds, targeting macOS 11 or later.

## Requirements

- macOS with the Xcode command-line tools installed.
- CMake 3.22 or later.
- A JUCE source checkout. Tested with JUCE 8.0.12, commit
  `29396c22c93392d6738e021b83196283d6e4d850`.

JUCE is an external dependency; set `JUCE_ROOT` to its checkout directory.
The plugin source, factory bank data, and required skin assets are included.
No original plugin installation is needed to build.

## Build

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DJUCE_ROOT=/path/to/JUCE
cmake --build build --config Release --target CamelPhatRebuilt_AU CamelPhatRebuilt_VST3 CamelPhatRebuilt_Standalone -j 6
```

Outputs are under `build/CamelPhatRebuilt_artefacts/Release/`:

- `AU/CamelPhat Rebuilt.component`
- `VST3/CamelPhat Rebuilt.vst3`
- `Standalone/CamelPhat Rebuilt.app`

Copy the AU into `~/Library/Audio/Plug-Ins/Components`, or the VST3 into
`~/Library/Audio/Plug-Ins/VST3`, then restart your music app and rescan plugins.

The editor and DSP are reconstructed implementations. Native output can differ
slightly from the Intel original because of floating-point rounding. The AU
uses host automation; MIDI CC input is available in VST3.
