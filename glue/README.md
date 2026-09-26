# glue/ — our C++ (the only C++ we write; upstreams stay pristine)

## Targets

- `offline-driver` — LIVE. Two-deck offline render (details below).
- `app/` — LIVE v1. Native Qt5 booth on `mixxx-lib`: two deck panels (load,
  file-BPM + SYNC, rate fader, keylock, KEY±, 3-band EQ, volume, jog,
  waveform + click-seek, CUE), shared transport, crossfader, master VU,
  arrangement timeline (mashup.json view + one-click Open arrangement),
  suggest-only copilot dock, all tooltips. Live sound via PortAudio;
  `mashpit-app --selftest song.wav` proves load→play→render headless;
  `--screenshot out.png [arr.json]` verifies layout headless.
  v1 limits: WAV/FLAC/OGG/OPUS (no MP3 decoder in this build config),
  hand-typed BPMs (or arrangement), no hotcues/loops/FX sends (omitted,
  not faked), timeline view-only, render stays in `mashpit render`.
- `analyze.cpp` → `mashpit-analyze` — LIVE. Real Mixxx analyzers
  (QueenMary beats, key, Ebur128 loudness) → `*.analysis.json` sidecars.
  `agent/mashpit analyze/edit/snippet/qc/undo` all live against it.

- `DjDeckTrack : Track` + `DjDeckPlayHandle : PlayHandle` — owns one Mixxx
  `EngineBuffer + CachingReader + scaler`; driven by LMMS
  `Song::processNextBuffer()`. Mixxx `SoundManager` never starts.
- `MixxxAnalyzerBridge` — import-time `AnalyzerBeats/Key/Gain` → sidecars.
- `XfaderEffect : Effect` — Mixxx crossfader curve + 3-band EQ as LMMS effect.
- `OfflineDriver` — no-clock render loop honoring the spike contracts
  (docs/SPIKE.md findings): pump-until-`track_loaded`, SAMPLES units,
  render-by-playposition with warmup trim, PreviewTier/FullTier resamplers.
- `glue/build/` (future) — documented patches for upstream trees
  (see docs/SPIKE.md §patches), applied at build time, never committed
  into upstream checkouts.

Build wiring: `MASHPT_GLUE_DIR` into the Mixxx tree (see docs/SPIKE.md);
`MASHPT_SPIKE_DIR` locates the scratch prefix + stubs until Phase 1b
proper (upstream/+prefix layout).
