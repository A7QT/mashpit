# mashpit

Offline mashup lab: **Mixxx's DJ engine on a DAW timeline.** DJ brains, DAW workflow,
no live pressure. GPL throughout — Mixxx (GPLv2+) + LMMS (GPLv2+) can legally merge.

> Status: prototypes only (`demo*.html`). The real build (C++ graft) hasn't started.
> These docs are the handoff to the agents that build it.

## Locked decisions (don't relitigate without new evidence)

- **2 decks, project-tempo-rules.** Project BPM is the boss; decks stretch to fit.
  2→4 decks later is copy-paste if `DjDeckTrack` is done right.
- **LMMS is the host, Mixxx is the organ.** LMMS owns clock, timeline, mixer,
  offline export. Mixxx owns decks, time-stretch, beat/key analysis, sync.
- **Copilot is suggest-only.** It emits typed edit ops, the human applies.
  No autonomous agent loop. Ever.
- **Same `mashup.json` renders the same WAV.** Determinism is a requirement,
  it's what makes the copilot correction loop work.

## Repo map

```
mashpit/
  README.md            you are here
  docs/                the real-deal specs (ARCHITECTURE, MIXXX_UI_HEIST, ROADMAP)
  demo.html            prototype v1 — agent-centric (FROZEN, do not touch)
  demo2.html           prototype v2 — structure/performer view (FROZEN, do not touch)
  demo3.html           prototype v3 — Mixxx-replica booth + tool UI (REFERENCE ONLY)
  app/                 interactive UI mockup (clickable spec for the real Qt UI —
                       the product itself is native, NOT a web app)
  upstream/            pinned pristine sources (pins.env + sync.sh — NOT
                       submodules: git fights shallow checkouts, full history
                       is gigabytes; same reproducibility, none of the weight)
  glue/                (future) our C++: DjDeckTrack, bridges, render driver
  spec/                (future) mashup.schema.json + fixtures
  agent/               (future) copilot MCP/CLI (mashpit_mcp)
```

## Run it (dev build — everything below is verified on this machine)

One-time per shell (vendored taglib lives outside the system paths):

```sh
export SPIKE=/home/david/Documents/projects/personal/mashpit-spike
export LD_LIBRARY_PATH=$SPIKE/prefix/lib64:$LD_LIBRARY_PATH
export MASHPT_OFFLINE_DRIVER=$SPIKE/mixxx-build/mashpit-glue/offline-driver
export MASHPT_ANALYZER=$SPIKE/mixxx-build/mashpit-glue/mashpit-analyze
```

The app (needs a working audio device for live sound):

```sh
$SPIKE/mixxx-build/mashpit-glue/app/mashpit-app
```

The CLI (no audio device needed):

```sh
./agent/mashpit validate spec/fixtures/demo-blend.json
./agent/mashpit render  spec/fixtures/demo-blend.json --tier preview
./agent/mashpit analyze ~/Music/song.wav        # needs real audio, not fixtures
./agent/mashpit edit    mashup.json --op match_bpm --dry-run
./agent/mashpit snippet mashup.json --from-sec 13 --to-sec 37
./agent/mashpit qc      render.wav
./agent/mashpit undo    mashup.json
```

Notes: app + fixtures load WAV/FLAC/OGG/OPUS (no MP3 decoder in this build
config — convert with `ffmpeg -i in.mp3 out.wav`). First full build lives
under `mashpit-spike/` per `docs/SPIKE.md`; a clean-room rebuild is
`upstream/sync.sh` + the same cmake flags.

## Environment (verified on this machine)

- Mixxx **2.4 branch** (Qt5, last Qt5 series) + LMMS **v1.2.2** (Qt5.15) — Qt5
  unity is load-bearing (Mixxx ≥ 2.5 is Qt6 and cannot link into an LMMS-Qt5
  binary). The distro Mixxx 2.5.6 install is reference/behavior only.
- System libs: `librubberband.so.3`, `libSoundTouch.so.2`
- Mixxx skins on disk: `/usr/share/mixxx/skins/` (LateNight is the reference skin)
- Mixxx DB (beatgrids/cues live here): `~/.mixxx/mixxxdb.sqlite`
- LMMS offline render already exists: `lmms render project.mmp -f wav -i sincbest`

## Docs for builders

- [`docs/ARCHITECTURE.md`](docs/ARCHITECTURE.md) — what gets grafted, `mashup.json` v1, engine mapping
- [`docs/MIXXX_UI_HEIST.md`](docs/MIXXX_UI_HEIST.md) — how to steal Mixxx's skin/UI code, widget by widget
- [`docs/ROADMAP.md`](docs/ROADMAP.md) — build phases with kill criteria and exit gates
