# Mixxx UI heist guide

We are lifting Mixxx's deck UI design — layout, controls, behaviors, tooltips —
into mashpit. Mixxx is GPLv2+ and so are we: keep license headers on anything
copied, note the origin file in a comment. This doc maps the loot.

Inspected on disk (verified): Mixxx **2.5.6** skins at
`/usr/share/mixxx/skins/`. Reference skin: **LateNight**. Engine graft target is
the **2.4 branch** (Qt5) — re-verify control parity there during the spike;
control names below come from 2.5.6 skins and are expected stable.

## 1. How Mixxx skins work (5-minute version)

- A skin is a directory of `.xml` files. Entry: `LateNight/skin.xml`.
- Layout is compositional: `decks/deck.xml` includes row files
  (`row_1_keyVinylFx.xml`, `row_2_3_TitleArtistTime.xml`,
  `row_4_overviewSpinny.xml`, `row_5_transportLoopJump.xml`), plus
  `rate_controls.xml`, `key_controls.xml`, `mixer/`, `fx/`, `controls/`.
- Reusable widgets are `<Template>`s with `<SetVariable name="...">` parameters.
  Example — `controls/button_hotcue.xml` takes only `number`; the deck instantiates
  it 8× with `<SetVariable name="number">1..8</SetVariable>`, and `Group` binds it
  to `[Channel1]` / `[Channel2]`. **Port this pattern, not the XML.**
- A widget binds to the engine via `<Connection><ConfigKey>Group,control</ConfigKey>`.
  `Group` is `[Channel1]`, `[Channel2]`, `[Master]`, `[Skin]`, or
  `[EffectRack1_EffectUnitN]`. Our port: each `Group,control` pair becomes one
  control in the booth with identical semantics (mapping tables below).
- `TooltipId` (e.g. `hotcue`, `sync_enabled`, `track_key`) is Mixxx's behavior
  glossary — every id names one documented behavior. Tooltip *text* lives in
  Mixxx source; when porting a widget, write the tooltip from the behavior,
  and state demo limits honestly (see §4).
- `[Skin],show_*` flags (`show_8_hotcues`, `show_loop_controls`,
  `show_beatjump_controls`, …) toggle widget visibility. Our port: feature flags
  or progressive disclosure, same names where practical.

## 2. Deck control inventory (verified from LateNight 2.5.6)

Group = `[Channel1]` / `[Channel2]`. Status = port state in `demo3.html`
(the UI reference implementation future work must match or exceed).

| Control | Mixxx behavior | demo3 control | Port status |
|---|---|---|---|
| `play` / `stop` | transport | ▶/■ per deck (shared transport underneath) | ✅ |
| `cue_default` | CUE: goto clip start / preview | CUE button | ✅ |
| `sync_enabled` | match + follow leader BPM | SYNC (+ editable file-BPM field feeding the math) | ✅ |
| `rate` + `rateRange`/`rate_dir` | pitch fader ±8% | vertical orange rate slider 92–108% | ✅ |
| `keylock` | tempo without pitch | KEYLOCK (detune compensation live, RubberBand at render) | ✅ |
| `pitch_adjust` | key shift in semitones | KEY−/KEY+ (±6, readout in `+N st`) | ✅ |
| `visual_key` / `file_key` | sounding vs file key | key readout `8A` + shift readout | ✅ |
| `sync_key` / `reset_key` | match/reset key to leader | KEY+ / KEY− + SYNC (reset on re-sync) | ✅ partial |
| `hotcue_1..8` (+activate/clear) | set / jump / clear | 8 numbered buttons: click=set, click=jump, alt-click=clear | ✅ |
| `beatloop_1/2/4/8` | loop N beats | LOOP 1/2/4/8 size buttons, click lit = release | ✅ |
| `loop_in/out`, `reloop_toggle` | manual loops | loop region `{start,end}` + rack clear | ✅ partial |
| `beatjump_1_forward/backward` (+N) | jump N beats | −4/−1/+1/+4 (quantized if Quant on) | ✅ |
| `pfl` | headphone cue | H button (solo in demo, one phones bus) | ✅ partial |
| main routing | deck → main mix | M button (mute-from-main, inverted display) | ✅ |
| `pregain` | pre-EQ trim | GAIN trim 0–200% | ✅ |
| 3-band EQ | isolator-style EQ | LOW/MID/HI ±12dB, live | ✅ |
| `volume` | channel fader | A/B faders + dB readouts | ✅ |
| `vu_meter` / `peak_indicator` | channel metering | master VU in booth center (per-deck meters = future) | ✅ partial |
| `bpm` / `rate_ratio` | tempo readouts | ratio readout (4 decimals) + master BPM | ✅ |
| `track_time` / `duration` | clocks | per-deck elapsed/remaining | ✅ |
| `quantize` | snap cue/loop to beat | global Quant toggle | ✅ |
| `slip_enabled` | slip mode | not ported | ❌ future |
| vinyl/jog `scratch` | touch vinyl | jog wheel: seek paused, pitch-bend playing (spring-back) | ✅ (no vinyl weight physics) |
| `sync_distance` / beatgrid | beat alignment display | beatgrid overlay on waveforms | ✅ partial |

## 3. Mixer + FX (LateNight `mixer/`, `fx/`)

- Channel strips (`channel_left/right.xml`, `eq_knob_*`): gain → EQ → fader →
  PFL → crossfader assignment. Our chain order matches (§ARCHITECTURE 4).
- `mixer_main_headphone.xml`: MAIN level, BAL, HEAD MIX / HEAD VOL, SPLIT.
  Port so far: MASTER knob, booth VU, crossfader (mirrored both directions).
  BAL + SPLIT + separate phones bus = future.
- FX rack (`fx/unit.xml`, `meta_knob.xml`, `parameter_knob.xml`,
  `[EffectRack1_EffectUnitN]` groups): effect units with meta + 3 parameter knobs
  and per-deck assign buttons. Port so far: master FILTER sweep + dub ECHO
  (dotted-8th, wet return) with per-deck FX SEND taps. Full effect units = future.
- FX1/FX2 badges in the deck header are currently decor — wire them to assign
  state when real effect units land.

## 4. Porting rules (learned from demo3)

1. **Steal behavior, not XML.** Qt skin XML doesn't run in a browser / LMMS —
   port the control's contract (range, default, persistence, tooltip).
2. **Every control gets a tooltip** stating what it does *and* its demo limit.
   Example: keylock tooltip admits detune-compensation-live/RubberBand-at-render.
   A tooltip that lies is a bug in the control, not the text.
3. **Keep Mixxx ids/names** for controls (`sync`, `keylock`, `hotcue_N`,
   `beatloop_N`, `beatjump_N`, `pfl`) in code and docs so future agents can
   grep Mixxx source and land in the right place here.
4. **Tempo-direction invariant:** ratio > 1 ⇒ plays faster. It was inverted once
   (`1/ratio`); the fix is load-bearing. Test it on every audio change.
5. **Verify like demo3 did:** markup parses, `node --check` clean, every
   `$('id')` resolves, no duplicate ids. Port these gates to C++ CI.

## 5. Conscious omissions (steal later, in order)

1. Per-deck VU meters + peak indicators (master VU exists).
2. Slip mode, vinyl weight/jog physics, needle-drop seeking.
3. Full FX units (meta + parameter knobs, assign matrix) — rack is filter+echo only.
4. 4-deck + 64-sampler skins (`LateNight (64 Samplers)` exists on disk — the
   layout pattern for scaling past 2 decks lives there).
5. `Deere`/`Tango`/`Shade` skins — alternate layouts worth mining for the
   timeline/DAW half, which LateNight doesn't cover.
