# Findings & Decisions

## Requirements

- Clone and analyze `ratwithacompiler/OBS-captions-plugin`.
- Create a modernization plan for an AI Caption Plugin fork.
- Start local development, not only produce a proposal.
- Verify a build against the current OBS release installed on this Windows machine.

## Research Findings

- Upstream HEAD is `ce3a76c` (2025-10-15), release `v0.33`; the project is GPL-2.0.
- The current upstream build is hand-wired around `OBS_BUILD_DIR`/`OBS_DEPS_DIR`, has no CMake presets, and requires either a compiled-in Google API key or a custom-key UI flag even to configure.
- GitHub's release API reports OBS 32.2.1 (2026-07-24) as current, and the locally installed executable is also 32.2.1. OBS 32 prevents loading plugins built for a newer OBS release.
- The official OBS plugin template targets Visual Studio 2022 and modern CMake on Windows.
- Issue #160's OBS 32 dock API break was fixed upstream, but issue #170 mixes plugin layout, platform security, network blocking, and actual load failures; packaging diagnostics are therefore a first-class requirement.
- Issue #176 confirms the hard-coded `en-US` default, while #178/#180 request an external or local model backend. This supports separating recognition from OBS capture/output before integrating a model runtime.
- The host has Visual Studio Build Tools 2022 (17.14), Windows SDK 22621/26100, CMake 4.3.4, and an OBS installation under `C:\Program Files\obs-studio`.
- The modernized source configures and builds successfully against pinned OBS 32.2.1, July 2026 OBS dependencies, and Qt 6.
- The resulting DLL imports only OBS/Qt/system runtime libraries; `plibsys` is linked statically and is not a runtime dependency.
- The inherited source has many narrowing, format-string, deprecated-network, and deprecated-Qt warnings. A real uninitialized debug flag was removed during the first refactor.
- A new `CaptionEngine` interface now separates audio submission and result callbacks from the concrete Google streaming implementation; a fake engine contract test passes.
- The dedicated factory now owns engine selection/construction and reports unsupported or failed construction through a typed result. An isolated portable OBS 32.2.1 run loaded `ai-caption-plugin.dll` and reached `obs_module_post_load` without a plugin-load error.
- sherpa-onnx 1.13.4 publishes a Visual Studio 2022 Windows x64 C API runtime and a Russian T-One streaming CTC model. The model uses CPU with one inference thread and expects 8 kHz input.
- The official model archive is 128,468,156 bytes and has SHA-256 `b9c907450e99a6e5049e279bf18368a17db0bdc5e63b7fa978943138debbe3ae`; the pinned runtime archive is SHA-256 `e33dc64195d17601879532583233d0d6ed76aa399eb863e5ca0783c5ac82b5aa`.
- On this PC, the official 6.4-second Russian sample decoded in 0.39 seconds with one CPU thread (RTF 0.062). The local engine's recognition worker is below-normal priority and has a bounded one-second queue, so it cannot block OBS audio delivery.
- The v0.35.0 installer was tested in a separate temporary plugin root: first install downloaded/validated/extracted the model; update retained the complete model without a second download.
- CEA-708 includes the P16 16-bit character path, and several decoders interpret it as a Unicode/UCS-2 code point. This makes Cyrillic technically encodable, unlike EIA-608.
- The current native output path cannot use that capability: `obs_output_output_caption_text2()` enters OBS's `caption_frame_from_text()` EIA-608 serializer, whose mapping does not include Cyrillic. The public `obs_output_caption()` raw-packet path also filters out packet types 2/3 (DTVCC start/data) in OBS 32.2.1 and forwards only type 0 (EIA-608) packets.
- Therefore native Russian Twitch CC requires a delivery layer beyond this plugin: a custom OBS build that preserves raw CEA-708 data, or a separately validated RTMP relay that injects CEA-708 `onCaptionInfo` metadata into the stream connection. Neither is safe to install or enable implicitly during a plugin update.
- OBS merged multitrack/HEVC/AV1 caption carriage specifically after testing with Twitch Enhanced Broadcasting (PR #10546), so Enhanced Broadcasting itself is not the reason ordinary native captions fail. Current OBS `master` still contains the raw-caption filter that discards DTVCC packet types 2/3. The OBS community also records a Restream re-encoding failure report; it is evidence of multistream risk, not a universal guarantee that every relay strips captions.
- No OBS Forum thread or OBS GitHub issue located a supported route for Unicode/P16 captions through a stock OBS build. OBS maintainers closed the CEA-compliance issue while acknowledging that its caption implementation remains substantially non-compliant and welcoming a contributor fix.
- Estimated OBS-core scope (not implemented): a transport-only repair is about 3-5 focused engineering days plus 1-2 days of integration testing. It must retain `cc_valid` and CEA-708 types 2/3, preserve packets beyond the 31-entry ITU-T T.35 limit for the next video frame, and test AVC/HEVC/AV1 plus Enhanced Broadcasting tracks. It would still require a plugin-side DTVCC/P16 packetizer.
- A product-grade OBS feature that makes `obs_output_output_caption_text2()` accept UTF-8 Russian text is a 3-6 week effort: add a maintainable DTVCC service/P16 encoder in libcaption, design an API/service/language fallback contract, integration/bitstream tests, and real Twitch acceptance tests. The largest external risk is unproven Twitch-player handling of P16 rather than encoding CPU cost.

## Technical Decisions

| Decision | Rationale |
|---|---|
| First prove upstream behavior, then refactor | A baseline build distinguishes modernization regressions from pre-existing failures. |
| Use an engine-neutral interface with Google as an adapter | Enables sherpa-onnx/SenseVoice or an external text feed without coupling them to OBS UI/audio code. |
| Keep the current Google HTTP adapter as the only factory selection | Preserves the verified behavior while making unsupported engines fail explicitly until M3 adds settings and UI selection. |
| Prefer the official OBS 32 SDK/dependency conventions | The existing custom CI scripts are difficult for contributors to reproduce. |
| Do not create a public GitHub fork yet | Local work can be validated before making an external repository under the user's account. |
| Keep warnings enabled but not fatal for the first baseline | The upstream warning backlog is large; turning it fatal hides actual OBS 32 compatibility results. |
| Do not install into the user's active OBS yet | The build/package check is complete; runtime loading should use an isolated profile and preserve the existing plugin. |
| Default to local Russian T-One in the packaged build | The user explicitly rejected Google API use and the model delivers streaming CPU-only recognition. |
| Download the model from a pinned official release during installation | The ZIP stays compact while first install remains automatic and integrity-checked. |
| Preserve complete models across updates | Avoids unnecessary repeat downloads while replacing only plugin binaries. |

## Issues Encountered

| Issue | Resolution |
|---|---|
| PowerShell could not redirect stderr for `obs64.exe --version` in the first probe | Use file metadata or `Start-Process` with explicit output files. |
| Ninja is absent | Visual Studio generator is available and officially supported. |
| CMake 4.3 rejected old FetchContent and plibsys policy defaults | Modernized FetchContent usage and declared the minimum compatibility policy. |
| First engine contract test exposed missing standard includes in `CaptionResult.h` | Made the public header self-contained. |
| Installed OBS is configured to always run elevated | Used a copied portable runtime and a fresh profile for the load test, leaving the normal OBS configuration and plugin folders untouched. |

## Resources

- https://github.com/ratwithacompiler/OBS-captions-plugin
- https://github.com/ratwithacompiler/OBS-captions-plugin/issues/170
- https://github.com/ratwithacompiler/OBS-captions-plugin/issues/176
- https://github.com/ratwithacompiler/OBS-captions-plugin/issues/178
- https://github.com/ratwithacompiler/OBS-captions-plugin/issues/180
- https://github.com/obsproject/obs-plugintemplate
- https://github.com/obsproject/obs-studio/releases/tag/32.1.2
- https://github.com/obsproject/obs-studio/releases/tag/32.2.1
- https://github.com/obsproject/obs-studio/pull/10546
- https://github.com/obsproject/obs-studio/issues/4006
- https://obsproject.com/forum/threads/closed-captioning-via-google-speech-recognition.108534/post-546285
