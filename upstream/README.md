# upstream/ — pristine third-party sources (NOT submodules, on purpose)

`git submodule` fights shallow clones (depth/branch checkout failures) and full
clones of Mixxx/LMMS history are gigabytes. So: pinned shallow clones via
`sync.sh`, verified by commit SHA. Same reproducibility, none of the weight.

## Pins (verified in the Phase 0 spike)

| Project | URL | Branch/tag | SHA |
|---|---|---|---|
| Mixxx | https://github.com/mixxxdj/mixxx.git | `2.4` (last Qt5 series) | see `pins.env` |
| LMMS | https://github.com/LMMS/lmms.git | `v1.2.2` | see `pins.env` |

Qt5 unity is load-bearing: Mixxx ≥ 2.5 is Qt6 and cannot link with LMMS-Qt5.

## Usage

```sh
./upstream/sync.sh /path/to/workdir   # shallow-clones both at pinned SHAs
```

Applies NO patches — trees stay pristine. Spike-only patches (CMake compat,
stubs, vendored taglib/GSL) are listed in `docs/SPIKE.md` and will become
proper build patches under `glue/build/` in Phase 1.

## Layout after sync

```
<workdir>/mixxx/    # Mixxx @ pinned 2.4 SHA
<workdir>/lmms/     # LMMS @ pinned v1.2.2 SHA
<workdir>/prefix/   # vendored taglib 1.13.1 + ms-gsl (built by glue/build/)
```
