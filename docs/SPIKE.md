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

## Build status

- [x] LMMS 1.2.2: configured + built, `lmms-build/lmms --version` runs (Qt 5.15).
- [ ] Mixxx 2.4: configured; `mixxx` target building (background).
- [ ] NEXT: isolated harness — construct Mixxx `EngineBuffer` + `CachingReader`
  + scaler with no `SoundManager`, drive `process()` with LMMS-sized blocks
  (`framesPerPeriod`) in a loop, write WAV, compare vs Mixxx's own recording.
  Gates: no `UNAVAILABLE` stalls, worker pools sane under burst pacing
  (see ROADMAP.md Phase 0 exit gate).
