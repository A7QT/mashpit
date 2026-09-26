# agent/ — copilot tooling (JSON in, JSON out; never touches audio)

- `mashpit` — CLI: `validate` + `render` (both live). `render` needs
  `MASHPT_OFFLINE_DRIVER` pointing at the built `glue/offline-driver`.
- `ops.py` — typed edit ops (`match_bpm`, `smooth_transition`, …) + `--dry-run` diffs.
- MCP server + snippet/qc pipeline land in Phase 2 (see docs/ROADMAP.md).

Rules: suggest-only, schema-validated output, 20-step undo. The copilot never
touches audio bytes or C++.
