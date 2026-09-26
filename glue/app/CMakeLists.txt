# mashpit app v1 (Qt5 booth on mixxx-lib). Built via the Mixxx spike tree:
#   cmake ... -DMASHPT_GLUE_DIR=<repo>/glue
# MASHPT_MIXXX_SRC = Mixxx source dir, MASHPT_SPIKE_DIR = scratch root
# (prefix/ includes+libs, spike-stub/ for gtest_prod.h). These paths get
# cleaned up when glue moves onto upstream/+prefix proper (Phase 1b).
find_package(Qt5 COMPONENTS Core Widgets REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(PORTAUDIO REQUIRED portaudio-2.0)
pkg_check_modules(SNDFILE REQUIRED sndfile)

set(MASHPT_SPIKE_DIR "" CACHE PATH "Spike scratch root (prefix/, spike-stub/)")

add_executable(mashpit-app
  main.cpp
  main_window.cpp
  deck_panel.cpp
  deck_engine.cpp
  jog_widget.cpp
  waveform_widget.cpp
  timeline_widget.cpp
  copilot_dock.cpp
  arrangement.cpp)
set_target_properties(mashpit-app PROPERTIES AUTOMOC ON)
target_include_directories(mashpit-app PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${MASHPT_MIXXX_SRC}
  ${MASHPT_SPIKE_DIR}/prefix/include
  ${MASHPT_SPIKE_DIR}/spike-stub
  ${PORTAUDIO_INCLUDE_DIRS}
  ${SNDFILE_INCLUDE_DIRS})
target_link_directories(mashpit-app PRIVATE
  ${MASHPT_SPIKE_DIR}/prefix/lib64)
target_link_libraries(mashpit-app PRIVATE
  mixxx-lib mixxx-gitinfostore Qt5::Core Qt5::Widgets
  ${PORTAUDIO_LIBRARIES} ${SNDFILE_LIBRARIES})
