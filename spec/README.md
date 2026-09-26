# spec/ — the `mashup.json` contract

- `mashup.schema.json` — v1 schema (the single contract between LLM, CLI,
  GUI and renderer; see docs/ARCHITECTURE.md §2).
- `fixtures/` — arrangements with known-good renders:
  - `demo-blend.json` — 2-track instrumental+vocal, xfade @ bar 9.
  - `demo-sonly.json` — single deck, no mix (degenerate case).
  - `demo-bad.json` — intentionally invalid (validator test).
