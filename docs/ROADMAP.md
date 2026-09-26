# mashpit roadmap

## Phase 0 — spike (days, kill-or-go)

Prove `EngineBuffer` runs inside an LMMS `PlayHandle` without `SoundManager`.
- Check out `upstream/mixxx@2.5.6` + `upstream/lmms@v1.2.2`, build both clean.
- Instantiate one Mixxx deck in a harness, feed it a file, call
  `EngineMixer::process()` in a loop, write WAV.
- **Kill criteria:** Qt thread-affinity (`ControlProxy`, `DbConnectionPool`)
  or the LRU/cache worker model can't survive outside Mixxx's callback thread
  after a serious attempt. Fallback if killed: JACK bridge (two apps, no merge).
- **Exit gate:** 8 bars rendered with LMMS-sized blocks (`framesPerPeriod`,
  not Mixxx callback sizes), no crashes, no `UNAVAILABLE` stalls, and
  `RubberBandWorkerPool` + `CachingReaderWorker` verified sane under
  faster-than-realtime burst pacing (or a documented workaround). Output
  compared against Mixxx's own recorded render of the same settings. This is
  the single riskiest integration in the system — nothing in later phases
  starts until this gate passes.

## Phase 1 — headless mash (the real milestone, no UI)

- `glue/` builds: `DjDeckTrack` + `OfflineDriver`.
- `mashpit render mashup.json out.wav` works end-to-end (see ARCHITECTURE §2).
- Force RubberBand Finer R3 + big blocks at render; `spec/mashup.schema.json`
  + `mashpit validate` with fix-hints.
- **Exit gate:** two-deck mix null-tests against a Mixxx live recording of the
  same settings within ±1dB (R2-vs-R3 difference budget), plus 3 fixture
  arrangements that render deterministically twice, bit-identical. Plus blind
  listen-tests calibrating the validator's ±6st / ±8% guesses into measured
  limits, marked TUNABLE-or-LOAD-BEARING in ARCHITECTURE.md.

## Phase 2 — ears for the copilot

- `MixxxAnalyzerBridge`: import → `AnalyzerBeats/Key/Gain` → `*.analysis.json`
  sidecars (ARCHITECTURE §3).
- Copilot CLI: `match_bpm`, `smooth_transition`, snippet MP3 previews (~15s,
  fast resample), `qc` (clipping/silence/LUFS/key-clash score), 20-step undo.
- The "make both match bpm + smooth the transition at 0:21" prompt works
  against a fixture and prints a human-readable diff first.
- **Exit gate:** full prompt→validate→preview→fix→full-render loop on fixtures,
  no human touching JSON.

## Phase 3 — the booth (UI from the heist)

- Rebuild `demo3.html`'s booth against the real engine, following
  MIXXX_UI_HEIST.md §2 until every row is ✅ (slip mode explicitly last).
- Timeline visualizes `mashup.json`: beatgrid overlay from `Beats::iterateFrom`,
  BPM/key badges, xfade zone, per-deck clocks.
- Copilot stays a docked suggest-only panel. Tooltips on everything, carried
  over from the prototype verbatim where behavior matches.
- **Exit gate:** a non-DJ human loads two tracks, SYNCs, smooths 0:21, and
  exports — using only tooltips, no help from us.

## Phase 4 — discord drop

- Demo projects + rendered WAVs/MP3s, one-click render, static build or AppImage.
- **Exit gate:** someone in discord renders a mashup without installing a toolchain.

## Frozen history (do not reopen)

- `demo.html` (agent-centric) and `demo2.html` (performer/structure view) were
  explored and rejected: agent-first and lifestyle-vibe respectively. The keeper
  direction is `demo3.html`: dense tool, Mixxx-replica booth, copilot as sidekick.
- License deliberation is over: GPL everywhere, private + discord distribution.
