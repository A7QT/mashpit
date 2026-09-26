# glue/ — our C++ (the only C++ we write; upstreams stay pristine)

## Planned targets (Phase 1)

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

Build wiring lands with the OfflineDriver slice (Phase 1b).
