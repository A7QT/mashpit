# agent/ — arrangement tooling: CLI for validate/render/analyze/edit/snippet/qc/undo

- `mashpit` — the whole CLI (stdlib only, except numpy for the layered
  render path). `render` needs `MASHPT_OFFLINE_DRIVER` pointing at the built
  `glue/offline-driver`; `analyze` needs `MASHPT_ANALYZER`.
- Sidecars live next to audio as `<file>.analysis.json` (written by `analyze`).

Rules: suggest-only edits, schema-validated I/O, 20-step undo. This tooling
assists arranging; the product is the timeline app. It never touches audio
bytes or C++ directly — it drives the engine through `mashup.json`.
