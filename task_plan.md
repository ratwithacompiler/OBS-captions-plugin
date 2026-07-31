# Task Plan: AI Caption Plugin modernization

## Goal

Deliver a reproducible Windows OBS 32 plugin with a local Russian streaming caption engine that does not use Google APIs, protects game/stream frame pacing, and downloads its model automatically on installation or upgrade.

## Next Step

Choose the delivery architecture for native Russian Twitch CC: a custom OBS build that forwards raw CEA-708 packets, or an independently tested local RTMP relay.

## Current Phase

Phase 7

## Phases

### Phase 1: Baseline and compatibility audit

- [x] Clone upstream and create an isolated work branch
- [x] Inventory the host OBS and Windows build tools
- [x] Review active compatibility and local-ASR issues
- [x] Record why the legacy OBS 30 build cannot validate current compatibility
- **Status:** complete

### Phase 2: Reproducible OBS 32 build

- [x] Replace ad-hoc CI assumptions with CMake presets/bootstrap documentation
- [x] Build a Windows x64 plugin against OBS 32.2.1 dependencies
- [x] Stage the modern OBS plugin directory layout
- **Status:** complete

### Phase 3: Caption engine boundary

- [x] Define an engine-neutral streaming caption interface
- [x] Store the existing Google engine through that interface
- [x] Add a deterministic fake engine test seam
- [x] Move engine selection and construction into a dedicated factory
- **Status:** complete

### Phase 4: Google baseline verification

- [x] Run compile/tests and inspect produced module dependencies
- [x] Document safe OBS installation and smoke-test steps
- [x] Record the next local-ASR milestone (sherpa-onnx/SenseVoice evaluation)
- [x] Run an isolated OBS module-load smoke test
- [ ] Run a live Google-caption smoke test with a valid API key and spoken audio (no longer required for the local-only package)
- **Status:** complete

### Phase 5: Local offline recognition and provisioning

- [x] Add a CPU-only sherpa-onnx T-One engine with one ONNX thread, a bounded audio queue, and a background worker below normal priority.
- [x] Make the local Russian engine the default while retaining the legacy Google adapter as an explicit compatibility option.
- [x] Persist engine selection and show model-install guidance in the settings UI.
- [x] Pin the official Windows runtime and Russian model archives with SHA-256 verification.
- [x] Add a Windows installer that safely updates only this plugin and downloads/validates the model on every install or upgrade.
- [x] Package the runtime DLLs and installer, then build/test and probe the packaged DLL with the OBS host runtime.
- **Status:** complete

### Phase 6: Streamer acceptance

- [ ] Install the packaged ZIP into the normal OBS plugin folder using its installer.
- [ ] Run a short PUBG/OBS stream and confirm CPU use, frame pacing, captions, and endpoint timing.
- **Status:** pending

### Phase 7: Native Russian Twitch closed captions

- [x] Verify that CEA-708 has a 16-bit P16 path that can carry Cyrillic code points.
- [x] Trace the packaged plugin's native output through OBS 32.2.1.
- [x] Establish that OBS 32.2.1 converts the plugin text API to EIA-608 and drops Cyrillic.
- [x] Establish that the public raw-caption API is filtered by OBS 32.2.1 to CEA-608 packet type 0 before video output.
- [x] Verify current OBS upstream work and community reports for Enhanced Broadcasting and multistreaming.
- [ ] Select a delivery architecture that can preserve raw CEA-708/P16 packets to Twitch without risking the primary stream.
- [ ] Implement and run a real Twitch viewer acceptance test with a Cyrillic probe.
- **Status:** blocked: upstream OBS supports ordinary captions across Enhanced Broadcasting tracks, but its current raw path still drops DTVCC packets. No supported plugin-only, no-relay route for Russian CC was found; the user has ruled out both remaining workarounds (a custom OBS build and a local relay).

## Key Questions

1. Can the current sources compile with MSVC 2022, Qt 6, and OBS 32.1.x without API changes?
2. Which failures are source incompatibilities versus packaging/dependency-layout problems?
3. What is the smallest interface that separates OBS audio/output logic from speech recognition?

## Decisions Made

| Decision | Rationale |
|---|---|
| Target Windows x64 and OBS 32.2.1 first | It exactly matches the installed OBS version and the current stable OBS release. |
| Preserve Google recognition as the first adapter | It gives a behavioral baseline while local engines are added incrementally. |
| Keep build credentials out of presets and source | API keys must remain local and must not enter Git history. |
| Use branch `codex/obs-32-modernization` | Keeps the cloned upstream default branch untouched. |
| Use sherpa-onnx T-One CTC for the first local engine | It is an officially supported Russian streaming model, has a CPU provider, and accepts one inference thread. |
| Install the model beside the plugin data, not in the ZIP | The 128 MB archive is downloaded only when absent or invalid and is preserved across plugin upgrades. |

## Errors Encountered

| Error | Attempt | Resolution |
|---|---:|---|
| Initial OBS version probe hit a PowerShell stderr-encoding issue | 1 | Re-run through a process wrapper or inspect executable metadata instead. |
| `ninja` is not on PATH | 1 | Use the supported Visual Studio 2022 generator on Windows. |
| CMake 4.3 rejected deprecated `FetchContent_Populate` | 1 | Switched to `FetchContent_MakeAvailable`. |
| Old plibsys declared compatibility below CMake 3.5 | 1 | Set the documented CMake policy compatibility floor to 3.5. |
| Inherited warnings were treated as errors by the OBS template defaults | 1 | Keep warnings visible but disable warning-as-error until the inherited backlog is reduced. |
