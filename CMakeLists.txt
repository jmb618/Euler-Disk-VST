cmake_minimum_required(VERSION 3.22)
project(EulerDisk VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(APPLE)
  set(CMAKE_OSX_DEPLOYMENT_TARGET "10.13" CACHE STRING "")
endif()

# ---------------------------------------------------------------- JUCE
# Soit un JUCE local :  cmake -B build -DJUCE_DIR=/chemin/vers/JUCE
# Soit telechargement automatique (necessite git + reseau).
if(DEFINED JUCE_DIR)
  add_subdirectory(${JUCE_DIR} JUCE)
else()
  include(FetchContent)
  FetchContent_Declare(JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        8.0.6
    GIT_SHALLOW    TRUE)
  FetchContent_MakeAvailable(JUCE)
endif()

# ---------------------------------------------------------------- Faust
find_program(FAUST_EXE faust)
if(NOT FAUST_EXE)
  message(FATAL_ERROR "Compilateur Faust introuvable. Installe-le (https://faust.grame.fr) et verifie que 'faust' est dans le PATH.")
endif()
execute_process(COMMAND ${FAUST_EXE} --includedir
                OUTPUT_VARIABLE FAUST_INCLUDE_DIR OUTPUT_STRIP_TRAILING_WHITESPACE)
message(STATUS "Faust : ${FAUST_EXE}  (includes : ${FAUST_INCLUDE_DIR})")

set(DSP_FILE   ${CMAKE_CURRENT_SOURCE_DIR}/dsp/euler_disk.dsp)
set(ARCH_FILE  ${CMAKE_CURRENT_SOURCE_DIR}/dsp/faust_arch.cpp)
set(GEN_DIR    ${CMAKE_CURRENT_BINARY_DIR}/generated)
set(GEN_FILE   ${GEN_DIR}/EulerDSP.hpp)

add_custom_command(
  OUTPUT  ${GEN_FILE}
  COMMAND ${CMAKE_COMMAND} -E make_directory ${GEN_DIR}
  # -double : calcul interne en double precision (compteurs, resonateurs a tres haut Q) ;
  # les entrees/sorties restent en float, le plugin n'a rien a changer.
  COMMAND ${FAUST_EXE} -double -lang cpp -cn EulerDSP -a ${ARCH_FILE} -o ${GEN_FILE} ${DSP_FILE}
  DEPENDS ${DSP_FILE} ${ARCH_FILE}
  COMMENT "Faust : euler_disk.dsp -> EulerDSP.hpp"
  VERBATIM)
add_custom_target(euler_dsp DEPENDS ${GEN_FILE})

# ---------------------------------------------------------------- Plugin
juce_add_plugin(EulerDisk
  COMPANY_NAME                "Pixel Lab"
  PRODUCT_NAME                "Euler Disk"
  FORMATS                     VST3 Standalone
  IS_SYNTH                    TRUE
  NEEDS_MIDI_INPUT            TRUE
  NEEDS_MIDI_OUTPUT           FALSE
  PLUGIN_MANUFACTURER_CODE    Pxlb
  PLUGIN_CODE                 Eudk
  VST3_CATEGORIES             Instrument Synth
  COPY_PLUGIN_AFTER_BUILD     FALSE)

target_sources(EulerDisk PRIVATE
  Source/PluginProcessor.cpp
  Source/PluginProcessor.h
  Source/PluginEditor.cpp
  Source/PluginEditor.h
  Source/ParamUI.h)

add_dependencies(EulerDisk euler_dsp)

target_include_directories(EulerDisk PRIVATE
  ${GEN_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/Source
  ${FAUST_INCLUDE_DIR})

target_compile_definitions(EulerDisk PUBLIC
  JUCE_WEB_BROWSER=0
  JUCE_USE_CURL=0
  JUCE_VST3_CAN_REPLACE_VST2=0
  JUCE_DISPLAY_SPLASH_SCREEN=0)

target_link_libraries(EulerDisk
  PRIVATE juce::juce_audio_utils
  PUBLIC  juce::juce_recommended_config_flags
          juce::juce_recommended_lto_flags
          juce::juce_recommended_warning_flags)
