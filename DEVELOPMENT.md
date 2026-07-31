# AI Caption Plugin development

This branch is a Windows-first modernization of `ratwithacompiler/OBS-captions-plugin`.
It builds reproducibly against OBS 32.2.1 and defaults to local Russian streaming recognition without a Google API key.

## Prerequisites

- Windows x64
- Visual Studio 2022 Build Tools with the C++ workload
- Windows SDK 10.0.22621 or newer
- CMake 3.28 or newer
- Git

Ninja and a Google API key are not required.

## Build and test

From the repository root:

```powershell
cmake --preset windows-x64
cmake --build --preset windows-x64 --parallel
ctest --test-dir build_x64 -C RelWithDebInfo --output-on-failure
```

The first configure downloads pinned OBS/Qt dependency archives and builds the OBS development libraries. Later runs reuse `.deps` and are much faster.

To stage the modern OBS plugin layout and create a ZIP:

```powershell
.\scripts\build-windows.ps1
```

The ZIP contains:

```text
ai-caption-plugin/
  Install-AICaptionPlugin.ps1
  local-model.json
  bin/
    64bit/
      ai-caption-plugin.dll
      ai-caption-plugin.pdb
      sherpa-onnx-c-api.dll
      onnxruntime.dll
      onnxruntime_providers_shared.dll
```

For normal installation or an update, extract the ZIP, close OBS, and run `Install-AICaptionPlugin.ps1`. It installs only the `ai-caption-plugin` directory under:

```text
%ProgramData%\obs-studio\plugins\
```

On first install, the script downloads the pinned Russian T-One model (about 128 MB), verifies the archive SHA-256, and writes per-file hashes beside the model. On subsequent updates it preserves a complete previously installed model. Keep the existing caption plugin installed separately until feature parity is verified. Do not commit API keys or a populated `CMakeUserPresets.json`.

## Local engine resource policy

- CPU provider only; the GPU and VRAM are never requested.
- One ONNX Runtime inference thread.
- OBS delivers 8 kHz mono PCM to the Russian T-One model.
- The OBS audio callback adds only to a bounded one-second queue; the local worker runs at below-normal Windows priority and drops old queued audio rather than delaying OBS.

## Current limitations

- The local Russian T-One streaming adapter is the packaged default. The legacy Google adapter remains compiled only as a compatibility path and its API-key UI is disabled in the standard preset.
- The inherited source has a sizeable compiler-warning backlog; warnings are recorded but are not yet treated as errors.
- The staged package passed the build/test pipeline, an isolated model-install/update smoke test, and an official 6.4-second Russian sample decode with one CPU thread (RTF 0.062). A full live-microphone OBS acceptance test remains to be performed by the streamer before a public release.
- The current build configuration is Windows x64 only.
