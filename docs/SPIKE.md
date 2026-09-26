# Phase 0 spike log

Scratch area (NOT in git, on big disk): `mashpit-spike/`
(`lmms/`, `mixxx/`, `taglib/`, `ms-gsl/`, `spike-stub/`, `*-build/`, `prefix/`).
Delete the whole folder to reset. Nothing here is precious — everything
reproducible is described below.

## Environment

- Fedora 44, GCC 16.2, CMake 4.3, 16 cores, Qt 5.15.18 + Qt 6.11 runtimes.
- Upstreams (shallow, `--depth 1`): `LMMS/lmms@v1.2.2`, `mixxxdj/mixxx@2.4`
  (branch head 62b93ea — **2.4 is the last Qt5 series**; distro Mixxx 2.5.6
  is Qt6 and cannot link with LMMS-Qt5, verified via `ldd`).
- Vendored into `prefix/`: taglib **1.13.1** (shared), Microsoft GSL **4.2.0**
  (header-only). System rest via dnf `-devel` packages.

## Patches applied to scratch copies (re-apply as real build patches later)

LMMS 1.2.2 (`lmms/CMakeLists.txt`, `lmms/src/CMakeLists.txt`):
1. `CMAKE_POLICY(SET CMP0026/CMP0050 OLD)` → `NEW` (CMake 4 removed OLD).
2. `CMAKE_POLICY_VERSION_MINIMUM=3.5` configure flag (2.8.7 minimum rejected).
3. `GET_TARGET_PROPERTY(BIN2RES … LOCATION)` → `SET(BIN2RES "$<TARGET_FILE:bin2res>")`.
4. `src/CMakeLists.txt`: `${SOUNDIO_LIBRARY}` out of the unconditional link
   list (1.2.2 links it unconditionally but only finds it conditionally);
   re-added under `IF(SOUNDIO_FOUND)`.
5. Configure flags: `-DWANT_CARLA=OFF -DWANT_VST=OFF -DWANT_GIG=OFF
   -DWANT_PORTAUDIO=OFF -DWANT_SOUNDIO=OFF -DWANT_SDL=OFF -DWANT_SNDIO=OFF`
   (drops the Carla submodule, Wine/VST, and backends we don't have).
6. `git submodule update --init src/3rdparty/qt5-x11embed src/3rdparty/rpmalloc/rpmalloc`.

Mixxx 2.4 (`mixxx/CMakeLists.txt`):
7. Test suite wrapped in `option(MIXXX_BUILD_TESTS … ON)` + `if()` (ends at
   `add_dependencies(mixxx-benchmark mixxx-test)`); resource `.qrc` lines for
   `mixxx-test` guarded the same way. Configure with `-DMIXXX_BUILD_TESTS=OFF`
   (no GTest on the box; our harness replaces the test suite for spike needs).
8. `find_package(Chromaprint REQUIRED)` → optional; when missing, defines a
   dummy `Chromaprint::Chromaprint` INTERFACE target (Findrubberband probes it)
   and compiles `../spike-stub/chromaprint_stub.cpp` (9 no-op functions,
   AcoustID self-disables at runtime — deck/analyzer paths never touch it).
9. `CMAKE_PREFIX_PATH=<prefix>` + `PKG_CONFIG_PATH=<prefix>/lib64/pkgconfig`
   so FindTagLib takes vendored 1.13.1 over system 2.3 (2.4 rejects TagLib ≥ 2.0).
10. `spike-stub/gtest/gtest_prod.h` stub (FRIEND_TEST macro only — production
    headers include it; real gtest/gmock belongs in Phase 1 CI, where Mixxx's
    own suite becomes the graft's regression net).
11. Vendored to `prefix/`: taglib **1.13.1** (shared), Microsoft GSL **4.2.0**
    (`ms-gsl-devel` doesn't exist on Fedora 44).

## Build status

- [x] LMMS 1.2.2: configured + built, `lmms-build/lmms --version` runs (Qt 5.15).
- [x] Mixxx 2.4: `mixxx` exe links and `--version` runs (2.4.2).
  NOTE: build with `CPLUS_INCLUDE_PATH=<spike>/prefix/include:<spike>/spike-stub`
  and `LIBRARY_PATH=<spike>/prefix/lib64` exported (spike includes don't
  propagate to the exe target's PCH through PRIVATE links).
- [x] PHASE 0 GATE: **PASS**. `spike-harness/harness.cpp` (scratch area):
  real file → `TestEngineMixer` (main/head/booth forced on) → real `Deck` →
  `Track::newTemporary` → `mixer.process(N)` with LMMS-sized 256-frame blocks,
  keylock on, WAV out via libsndfile. Non-zero exit on stall or silence.
  Run: `QT_QPA_PLATFORM=offscreen spike-harness song.wav out.wav 2000 256`.
  Result: WAV-vs-input correlation **1.0000** at 1.0x; steady state
  bit-identical across runs (only the first ~5% differs — see finding 3).
- [ ] Follow-ups (NOT gate-blockers): MP3 decode missing in spike config
  (no MAD/FFmpeg dev libs — WAV proved the clock path; MP3 is packaging
  for Phase 1); `spike-stub/` tracers + harness `fprintf` debug stay as-is.

## Phase 0 findings (load-bearing for the real build)

1. **Pump-then-render.** Track load completes on `CachingReaderWorker`, which
   only runs when `EngineMixer::process()` pumps the worker scheduler (~700
   blocks to load here). The driver must pump (paused) until `track_loaded=1`
   (bounded spin + timeout), THEN play and render. A fixed settle-sleep does
   nothing — the wake comes from `process()`, not wall time.
2. **Units: `EngineMixer::process()` takes SAMPLES, not frames** (stereo: ×2).
   Passing frames rendered half-speed output and confused every measurement
   until caught. Always name the unit at call sites.
3. **Warmup transient is nondeterministic; steady state is bit-identical.**
   First ~100 blocks vary run to run (cache-warm `UNAVAILABLE` race); after
   that, output is sample-identical. OfflineDriver must render by target
   *playposition* and trim (or pre-roll past) the transient — never assume
   block N maps to file position N before steady state.
4. **Centered crossfader = −3.03 dB.** Output RMS matched input × cos(π/4)
   exactly. Sanity-check gains against this whenever a render sounds "quiet".
5. **Qt qDebug/qCritical are mute in this shell env** (proven with a minimal
   Qt app — fprintf works). Harness diagnostics use fprintf(stderr); the WAV
   is the source of truth. Don't "fix" this in the harness — fix nothing.
6. **moc needs macro definitions on its include path.** A stub header alone
   isn't enough: without the include dir in `MOC_INCLUDES`, moc emits broken
   `FRIEND_TEST` slot code. Verified via `AutogenInfo.json` inspection.
