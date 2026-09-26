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

## Phase 2 — ears (analysis sidecars + assistant commands)

- `MixxxAnalyzerBridge`: import → `AnalyzerBeats/Key/Gain` → `*.analysis.json`
  sidecars (ARCHITECTURE §3). Done: `mashpit-analyze` + `mashpit analyze`.
- Assistant CLI (`match_bpm`, `smooth`, snippet previews, `qc`, undo): a
  sidekick for arranging faster — explicitly NOT the product. Done, tested.
- **Exit gate:** sidecars for fixtures are correct (BPM/key/gain sane);
  `render` consumes sidecar BPM for auto-ratio; full
  analyze→edit→preview→qc→undo loop runs without touching JSON by hand.

## Phase 3 — the timeline app (the actual product)

- Booth against the real engine, following MIXXX_UI_HEIST.md §2 until every
  row is ✅ (slip mode explicitly last). Done in `glue/app` v1 (jogs,
  waveforms, CUE, EQ, crossfader, live PortAudio).
- Timeline: `mashup.json` as an editable arrangement — draggable clips,
  beatgrid overlay from `Beats::iterateFrom`, BPM/key badges, xfade zone,
  per-deck clocks. View-only today; dragging clips IS the core remaining work
  (DjDeckTrack), ahead of all assistant features.
- Assistant stays a docked suggest-only panel. Tooltips on everything.
- **Exit gate:** a non-DJ human loads two tracks, drags them on the timeline,
  SYNCs, smooths 0:21, and exports — using only tooltips, no help from us.

## Phase 4 — discord drop

- Demo projects + rendered WAVs/MP3s, one-click render, static build or AppImage.
- **Exit gate:** someone in discord renders a mashup without installing a toolchain.

## Frozen history (do not reopen)

- `demo.html` (agent-centric) and `demo2.html` (performer/structure view) were
  explored and rejected: agent-first and lifestyle-vibe respectively. The keeper
  direction is `demo3.html`: dense tool, Mixxx-replica booth, copilot as sidekick.
- License deliberation is over: GPL everywhere, private + discord distribution.
- Scope correction (Sep 2026): this is **timeline-based Mixxx, not an AI
  mashup maker**. The assistant/copilot is a sidekick panel and a CLI;
  the product is the booth + timeline + render. Any doc language suggesting
  otherwise is stale — fix it on sight.
