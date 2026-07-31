# Progress Log

## Session: 2026-07-31

### Phase 1: Baseline and compatibility audit

- **Status:** complete
- **Started:** 2026-07-31
- Actions taken:
  - Cloned upstream into `outputs/AI-Caption-Plugin`.
  - Created branch `codex/obs-32-modernization` and renamed the source remote to `upstream`.
  - Inventoried repository layout, CMake configuration, installed toolchain, and selected GitHub issues.
  - Confirmed the host has OBS, MSVC Build Tools 2022, two Windows SDKs, Git, and CMake.
  - Confirmed both installed and current upstream OBS are version 32.2.1.
- Files created/modified:
  - `task_plan.md`
  - `findings.md`
  - `progress.md`

### Phase 2: Reproducible OBS 32 build

- **Status:** complete
- Actions taken:
  - Added the official OBS plugin CMake bootstrap modules and Windows x64 presets.
  - Pinned OBS 32.2.1, OBS dependencies 2026-07-15, Qt 6 dependencies 2026-07-15, and plibsys commit `fd23c00` with hashes where distributed.
  - Built and staged `ai-caption-plugin.dll` in the current per-plugin layout.
  - Added a one-command build/test/package script.

### Phase 3: Engine boundary

- **Status:** complete
- Actions taken:
  - Added `CaptionEngine` and made the current Google streaming implementation conform to it.
  - Changed `SourceCaptioner` to store an engine-neutral pointer.
  - Added and passed a fake-engine contract test.
  - Added a caption-engine factory with explicit engine selection and a typed construction result; the Google HTTP adapter is the currently supported selection.
  - Removed a read of an uninitialized debug variable.

### Phase 4: Verification and hand-off

- **Status:** in_progress
- Actions taken:
  - Inspected PE imports: only Qt 6, OBS, Windows, and MSVC runtime DLLs are required.
  - Generated `release/AI-Caption-Plugin-0.34.0-windows-x64.zip`.
  - Documented local build, safe installation, limitations, and modernization milestones.
  - Loaded the staged DLL in an isolated, portable OBS 32.2.1 instance; the fresh log records both `obs_module_load` and `obs_module_post_load` without an AI Caption Plugin load error.
- Pending:
  - Live caption output with a valid Google API key and spoken test audio. No key was read from, or written to, the isolated profile.

### Phase 5: Local offline recognition and provisioning

- **Status:** complete
- Actions taken:
  - Added `SherpaTOneCaptionEngine`, a CPU-only Russian streaming CTC adapter using the official sherpa-onnx 1.13.4 C API.
  - Added a one-second bounded audio queue, one ONNX inference thread, 8 kHz PCM input, endpoint handling, and below-normal Windows worker priority.
  - Made the local engine the default, migrated local settings to `ru-RU`, and removed the Google API-key field from the standard Windows preset.
  - Pinned the official sherpa-onnx Windows runtime and Russian model archives with SHA-256 verification.
  - Added `Install-AICaptionPlugin.ps1`; it elevates only for the normal OBS plugin folder, blocks installation while OBS runs, stages/rolls back only this plugin, downloads/verifies the model on first install, and preserves a complete model on update.
  - Packaged v0.35.0 with the three required local runtime DLLs and the installer.

### Phase 7: Native Russian Twitch closed captions

- **Status:** blocked pending delivery-architecture choice.
- Actions taken:
  - Verified that CTA-708's P16 path can represent a 16-bit Cyrillic code point.
  - Traced the plugin's native output through OBS 32.2.1: the simple text API serializes through EIA-608, which rejects Cyrillic.
  - Verified that the public OBS raw-caption API currently discards CEA-708 DTVCC packet types before output, retaining only type-0 EIA-608 data.
  - Verified OBS PR #10546: ordinary captions are intentionally copied to every Enhanced Broadcasting video track, including HEVC and AV1 transport support. Verified that the DTVCC filter remains in current OBS `master`.
  - Reviewed OBS Forum and GitHub reports: no stock-OBS Unicode/P16 route was found; one historical Restream report experienced re-encoding problems while direct Twitch/YouTube did not, so multistream relays cannot be assumed to preserve embedded captions.
- Result:
  - Enhanced Broadcasting is compatible with ordinary native CC, but a plugin-only source change still cannot make the existing OBS installation transmit native Russian CC to Twitch.
  - The only identified paths remain an OBS core patch/custom distribution or a local RTMP relay, followed by a real viewer-side Twitch CC test. The user rejected both operational paths, so this phase remains blocked pending upstream OBS support.
  - Scope assessment: an upstream raw-CEA-708 transport repair is roughly one engineering week including integration validation, but needs a separate plugin packetizer. First-class UTF-8 Russian support in OBS is a 3-6 week feature with substantial interoperability risk; it cannot be responsibly promised until a Twitch P16 viewer probe succeeds.

## Local-ASR Test Results

| Test | Input | Expected | Actual | Status |
|---|---|---|---|---|
| Local engine build | VS 2022, RelWithDebInfo | Plugin links sherpa-onnx | Built successfully | PASS |
| Contract test | Missing local model path | Typed construction failure, no crash | 1/1 CTest passed | PASS |
| First install smoke | Isolated temporary OBS plugin root | Download, hash check, extract model | Downloaded 128,468,156-byte archive and installed `model.onnx`/`tokens.txt` | PASS |
| Update install smoke | Same isolated root | Preserve model without another download | Verified model was retained | PASS |
| Local decode | Bundled 6.4 s Russian sample | CPU-only transcription | Correct Russian text; one thread; RTF 0.062 | PASS |
| Packaged DLL load probe | Isolated OBS runtime DLL search path | Runtime dependencies resolve | sherpa-onnx and packaged plugin DLL load after OBS host libraries | PASS |

## Test Results

| Test | Input | Expected | Actual | Status |
|---|---|---|---|---|
| Repository clone | Upstream Git URL | Clean clone | Clean clone at `ce3a76c` | PASS |
| Work branch | `codex/obs-32-modernization` | Separate branch | Branch active | PASS |
| Windows compiler toolchain | VS Build Tools probe | VS 2022 available | 17.14 Build Tools installed | PASS |
| OBS 32.2.1 configure | Pinned sources/dependencies | Configure succeeds | Succeeded | PASS |
| Windows x64 build | RelWithDebInfo | DLL links | `ai-caption-plugin.dll` produced | PASS |
| Caption engine contract | Fake engine | Callback receives result | 1/1 test passed | PASS |
| PE dependency audit | Built DLL | No undeclared third-party DLL | OBS, Qt, Windows, MSVC only | PASS |
| Package script | Full cached pipeline | ZIP produced | ZIP produced | PASS |
| Isolated OBS module load | Portable OBS 32.2.1 profile | DLL appears in `Loaded Modules` and reaches post-load | `ai-caption-plugin.dll` loaded; `obs_module_post_load` recorded | PASS |

## Error Log

| Timestamp | Error | Attempt | Resolution |
|---|---|---:|---|
| 2026-07-31 | PowerShell stderr encoding blocked direct OBS version probe | 1 | Read executable metadata and verified the release through GitHub's API. |
| 2026-07-31 | `ninja` not found | 1 | Use Visual Studio 17 2022 generator. |
| 2026-07-31 | Deprecated FetchContent call rejected by CMake 4.3 | 1 | Replaced with `FetchContent_MakeAvailable`. |
| 2026-07-31 | Old plibsys CMake policy compatibility rejected | 1 | Set `CMAKE_POLICY_VERSION_MINIMUM=3.5`. |
| 2026-07-31 | Inherited warnings failed the first baseline | 1 | Disabled warning-as-error while retaining warnings in output. |
| 2026-07-31 | Contract test found missing `<chrono>` in a public header | 1 | Made `CaptionResult.h` self-contained. |

## 5-Question Reboot Check

| Question | Answer |
|---|---|
| Where am I? | Phase 4: only the credentialed live-caption smoke test remains. |
| Where am I going? | Measured local-ASR proof of concept after the credentialed Google baseline. |
| What's the goal? | A buildable local AI Caption Plugin fork scaffold. |
| What have I learned? | See `findings.md`. |
| What have I done? | See this session log. |
