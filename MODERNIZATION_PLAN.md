# AI Caption Plugin modernization plan

## Target architecture

```text
OBS audio capture (8 kHz mono PCM for local Russian engine)
            |
            v
      CaptionEngine
       /         \
Google adapter   Local sherpa-onnx T-One adapter
                      |
              CPU-only Russian streaming CTC
            |
            v
       CaptionResult
            |
            +--> Twitch/OBS native captions
            +--> OBS text sources
            +--> transcript files
```

`CaptionEngine` owns the streaming audio/result contract. Recognition adapters must not know about OBS scenes, docks, Twitch output, or transcript formatting.

## Milestones

### M0 — Reproducible OBS 32 baseline (complete)

- Pin OBS 32.2.1, the matching July 2026 OBS dependencies, and Qt 6.
- Build with Visual Studio 2022 through CMake presets.
- Package in the OBS 32 per-plugin directory layout.
- Keep secrets out of source and presets.

Acceptance: clean configure, successful x64 DLL build, dependency inspection, and repeatable package generation.

### M1 — Engine boundary (complete)

- Store the active recognizer through `CaptionEngine` rather than the Google implementation type.
- Prove the callback/audio contract with a deterministic fake-engine test.
- Move Google construction into a dedicated adapter/factory and give errors a typed status channel.

Acceptance: OBS capture/output code builds unchanged with either the Google adapter or a fake adapter. The factory currently exposes the legacy Google HTTP adapter and rejects unavailable engine types explicitly; M3 will add user-facing selection.

### M2 — Local ASR proof of concept (complete)

- Integrate sherpa-onnx 1.13.4 and the streaming Russian T-One CTC model through the C API.
- Fix the execution provider to CPU and the inference count to one thread; move recognition to a bounded below-normal-priority worker.
- Pin the official runtime/model archives with SHA-256 validation and package their runtime DLLs separately from the model.
- Verify the official 6.4-second Russian sample at one thread: 0.39 seconds, real-time factor 0.062.

Acceptance: configure/build/test succeeds; the installer downloads and validates the model on first install and retains a complete model during an update.

### M3 — User-facing engine selection

- Add Google/Local/External engine selection and engine-specific settings pages. The current package deliberately defaults to local Russian recognition and hides Google API settings.
- Add system-locale default with explicit language override.
- Surface model download state, network errors, and recognition health in the dock.

Acceptance: switching engines never requires restarting OBS and never changes output routing settings.

### M4 — Reliability and release

- Add reconnect/cancellation tests, audio-buffer backpressure limits, and clean shutdown tests.
- Resolve high-value inherited warnings, especially narrowing conversions and deprecated Qt signals.
- Add GitHub Actions builds, signed checksums, and Windows package smoke tests.
- Validate Twitch CC, recording captions, open captions, and transcript output on OBS 32.

Acceptance: a release candidate survives a two-hour stream, OBS shutdown/restart, network loss, and model failure without a crash or unbounded memory growth.

## Explicitly deferred

- Multiple simultaneous caption sources
- Translation
- macOS/Linux packaging

These follow a stable single-engine Windows release so they do not obscure core lifecycle and latency defects.
