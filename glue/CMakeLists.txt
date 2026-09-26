# mashpit glue/ build (Phase 1b+). Built against pinned upstream Mixxx
# sources (see upstream/pins.env). Standalone for now; LMMS-side targets
# (DjDeckTrack) land with the DjDeckTrack slice.
cmake_minimum_required(VERSION 3.16)
find_package(Qt5 COMPONENTS Core Widgets REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(SNDFILE REQUIRED sndfile)

add_executable(offline-driver offline_driver.cpp)
target_include_directories(offline-driver PRIVATE
  ${MASHPT_MIXXX_SRC}
  ${SNDFILE_INCLUDE_DIRS})
target_link_libraries(offline-driver PRIVATE
  mixxx-lib mixxx-gitinfostore Qt5::Core Qt5::Widgets ${SNDFILE_LIBRARIES})

add_subdirectory(app)
