# mashpit architecture

## 1. The graft

LMMS 1.2.2 is the body (clock, timeline, mixer, offline export).
Mixxx 2.5.6 is the transplanted brain (decks, stretch, analysis, sync).
One new layer of glue holds them together. Both upstreams stay pristine
as git submodules; **all our code lives in `glue/`, `spec/`, `agent/`**.

```
upstream/mixxx  @ 2.4 branch  (Qt5 — last Qt5 series; ≥2.5 is Qt6, unlinkable)
upstream/lmms   @ v1.2.2      (Qt5.15)
```

### Keep from Mixxx (`src/` paths in upstream)

| Module | Path | Why |
|---|---|---|
| Deck logic | `engine/enginebuffer.*` + `engine/controls/` (`RateControl, BpmControl, SyncControl, LoopingControl, CueControl, KeyControl`) | cue/loop/sync/key behavior, battle-tested |
| Time-stretch | `engine/bufferscalers/` (`EngineBufferScaleST`, `EngineBufferScaleRubberBand`, `Linear`) | tempo without pitch; R3 Finer offline = better than live Mixxx |
| Decode cache | `engine/cachingreader/`, `engine/readaheadmanager.*` | gapless decode, loop wrap, quantized seeks |
| Analyzers | `analyzer/` (`AnalyzerBeats` QM + SoundTouch, `AnalyzerKey`, `AnalyzerWaveform`, `Beats` grid/map) | BPM/key/beatgrid for every import |
| Mix helpers | `engine/channels/enginedeck.cpp`, `engine/enginemixer.cpp` (reference) | per-deck chain order is the spec |
| FX | `engine/effects/` (`EngineEffectsManager`) | EQ/filter as LMMS `Effect` later |
| Persistence | `track/`, `library/dao/` (`mixxxdb.sqlite`, `AnalysisDao`, `CueDAO`) | beatgrids/cues already have a schema — reuse, don't reinvent |
| Recording | `engine/sidechain/enginerecord.cpp` | closest thing to an offline renderer; study the `E -> R` resample path |

### Drop / stub from Mixxx

`soundio/soundmanager.cpp` + PortAudio/JACK (realtime callback — replaced by the
offline driver), controllers (`controllers/`), vinyl DVS (`VinylControlControl`),
skins GUI, broadcast, mic/talkover. LMMS `AudioEngine` is the only clock.

### Keep from LMMS

`Engine` singleton, `Song : TrackContainer`, `AudioEngine` (**`renderOnly` +
`AudioDummy` already exist**), `ProjectRenderer`/`RenderManager` (offline WAV/OGG/
FLAC/MP3 export — the "not on-the-fly" payoff), `SampleTrack` (reference for the
new track type), `AutomationTrack` (crossfader curves), `Mixer`/`MixerChannel`.

### The glue (`glue/`, the only C++ we write)

- `DjDeckTrack : Track` + `DjDeckPlayHandle : PlayHandle` — owns one Mixxx
  `EngineBuffer + CachingReader + scaler`. `Song::processNextBuffer()` drives it;
  Mixxx `SoundManager` never starts.
- `MixxxAnalyzerBridge` — runs Mixxx analyzers on import, writes sidecars ( §3 ).
- `XfaderEffect : Effect` — Mixxx crossfader curve + 3-band EQ as an LMMS effect.
- `OfflineDriver` — loops `EngineMixer::process()` block-by-block into
  `EngineSideChain`/encoder. No audio clock. Two tiers, selected per render:
  `PreviewTier` (SoundTouch or RubberBand Faster R2 + `sincfastest`) for the
  copilot's snippet loop — cheap but still deterministic; `FullTier`
  (RubberBand Finer R3 + `sincbest`) for final bounces. R3 on every render
  would stall the copilot loop on long projects, so previews never use it.
- `Clock & cache (open question for Phase 0)` — `CachingReader` /
  `ReadAheadManager` are demand-driven (`read()` / `hintAndMaybeWake()`), not
  wall-clock-driven, so LMMS-driven blocks should feed them; but `UNAVAILABLE`
  (cache miss → retry next callback) and the `CachingReaderWorker` QThread
  assume realtime pacing. Offline advantages to exploit: pre-warm hints for the
  *whole arrangement* up front (the timeline is fully known — live DJing never
  has this), enlarge the chunk cache, and possibly bypass the worker thread if
  local disk decode outruns consumption. Spike both, measure, lock one.

## 2. `mashup.json` v1 — the single contract

LLM, CLI, GUI and renderer all speak this. Versioned, JSON-Schema-validated
(`spec/mashup.schema.json`). Bars are musical truth; seconds are derived via
project BPM (`secPerBar = 60/bpm*4`). The LLM never does time math.

```json
{
  "version": 1,
  "projectBpm": 128,
  "tracks": [
    {"id": "inst", "file": "a.mp3", "role": "instrumental"},
    {"id": "vox",  "file": "b.mp3", "role": "vocal"}
  ],
  "clips": [
    {"track": "inst", "atBar": 1, "bars": 16, "tempoRatio": 1.0, "pitchSemi": 0},
    {"track": "vox",  "atBar": 9, "bars": 16, "tempoRatio": 1.02, "pitchSemi": -1}
  ],
  "mix": [
    {"atBar": 9, "bars": 8, "xfade": "equal-power",
     "eq": {"low": -2, "mid": 0, "high": 1}}
  ]
}
```

Validator rejects before render: clip overlap on one deck, `|pitchSemi| > 6`,
xfade with no clip overlap, unknown file, `tempoRatio` beyond ±8% without keylock.
Errors return as fix-hints the copilot consumes (`mashpit validate`).
The ±6st / ±8% numbers are **conservative musicality guesses, not measured
RubberBand cliffs** — quality degrades gradually, and these exist so results
still sound like the same tracks. Treat as TUNABLE (Phase 1 listen-tests
calibrate them into real limits); the load-bearing invariants are determinism
and ratio > 1 ⇒ faster.

## 3. Analysis sidecars (`*.analysis.json`, written once per file)

So the copilot "digs in crates" by reading JSON, never audio:

```json
{"file": "a.mp3", "bpm": 124.0, "key": "8A", "beats": "grid|map",
 "downbeats": [..], "loudnessLUFS": -9.2, "durationBars@128": 32.0}
```

Compatibility without listening: BPM delta < 3%, Camelot distance ≤ 1,
vocal entry on a downbeat. Source: Mixxx `AnalyzerBeats/Key/Gain` via the bridge.

## 4. Engine mapping (Mixxx control → real behavior)

Verified against Mixxx 2.5.6 source + LateNight skin (see MIXXX_UI_HEIST.md).
`[ChannelN]` = deck N.

| Mixxx control | Meaning | mashpit implementation |
|---|---|---|
| `rate` / `pitch` / `sync_enabled` | speed, fine pitch, follow-leader | `tempoRatio = projectBpm / fileBpm`; SYNC sets it + keylock |
| `keylock` | tempo without pitch | RubberBand Finer @render; detune compensation @monitor |
| `pitch_adjust` (±st) | key shift, tempo held | semitone shift, `\|k\| ≤ 6` enforced by validator |
| `hotcue_N_activate/clear` | set/jump/clear cue N | absolute-time cues; click=set, click=jump, alt-click=clear |
| `beatloop_N_activate`, `loop_in/out`, `reloop_toggle` | loops | loop region `{start, end}`; wrap by re-driving the renderer |
| `beatjump_N_forward/backward` | move playhead N beats | `S.t ± N·(60/bpm)`, quantized if Quant on |
| `cue_default` | CUE | jump to deck clip start |
| `pfl` / `orientation`+mute | phones cue / main out | solo / mute (one phones bus in the demo, split later) |
| `pregain`, `volume`, EQ | gain staging | trim → 3-band → fader → xfade → master (that order, always) |
| `bpm`, `visual_key`, `file_key` | analysis readouts | sidecars; editable file-BPM field feeds SYNC math |
| `track_time`, `duration` | clocks | elapsed/remaining per deck clip |
| `vu_meter` | meters | analyser on master (monitor), mixer metering at render |

Tempo-direction invariant (burned us once): ratio > 1 means the file plays
**faster** (`playbackRate = ratio`). Never invert it again.

Chain order (both monitor and render): `src → trim → low → mid → high →
vol → xfade(equal-power) → master → filter → destination`, echo send tapped
post-xfade per deck, dotted-8th delay, wet return to master.

## 5. Copilot contract

- Input: free text + `get_state` (compact: project BPM, per-track
  `{bpm, key, tempoRatio}`, mix points — never full beatmaps).
- Output: **only** typed edit ops: `match_bpm{scope}`, `smooth_transition{atSec,
  bars, curve}`, `move_clip`, `set_pitch`, `set_gain`. All schema-validated.
- Flow: `prompt → --dry-run diff (human reads) → snippet render (MP3 ~15s,
  `PreviewTier`) → full render (WAV, `FullTier`) on approval`.
- Undo: 20-step history of arrangement snapshots. Non-negotiable.
- The copilot never touches audio bytes or C++. JSON in, JSON out.

## 6. Conventions for agents working here

- GPL headers on every new source file (project is GPL, no exceptions to track).
- Never edit `upstream/`. Glue only.
- UI rule, inherited from the prototypes: **every control gets a tooltip**,
  and tooltips state honest limits ("browser-grade detune, RubberBand at render").
- Verify like the prototypes did: markup parses, JS `node --check`s clean,
  every `$('id')` resolves, no duplicate ids. Port these checks to C++ CI
  (build + render a fixture `mashup.json`, null-test vs Mixxx recording ±1dB).
