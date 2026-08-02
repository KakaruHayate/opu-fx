cmake_minimum_required(VERSION 3.20)
project(opu-fx VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT DEFINED IPLUG2_DIR)
  set(IPLUG2_DIR "${CMAKE_CURRENT_SOURCE_DIR}/iPlug2" CACHE PATH "iPlug2 root")
endif()

include(${IPLUG2_DIR}/iPlug2.cmake)
find_package(iPlug2 REQUIRED)

# Shared DSP header-only library
add_library(opufx_dsp INTERFACE)
target_include_directories(opufx_dsp INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/src/dsp)

if(APPLE)
  foreach(PLUGIN_NAME OO_EQ OO_Comp OO_Reverb OO_TrackPolish)
    configure_file(resources/macOS-Info.plist.in resources/${PLUGIN_NAME}-macOS-Info.plist @ONLY)
    configure_file(resources/VST3-Info.plist.in resources/${PLUGIN_NAME}-VST3-Info.plist @ONLY)
  endforeach()
endif()

# Helper: after iplug_add_plugin creates _<name>-base, attach DSP include
function(add_opu_plugin NAME)
  cmake_parse_arguments(ARG "" "" "SOURCES" ${ARGN})
  iplug_add_plugin(${NAME}
    SOURCES ${ARG_SOURCES}
    FORMATS VST3
    LINK opufx_dsp
  )
  target_include_directories(_${NAME}-base INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/src/dsp)
endfunction()

add_opu_plugin(OO_EQ
  SOURCES
    src/OO_EQ/OO_EQ.cpp
    src/OO_EQ/OO_EQ.h
    src/OO_EQ/config.h
)

add_opu_plugin(OO_Comp
  SOURCES
    src/OO_Comp/OO_Comp.cpp
    src/OO_Comp/OO_Comp.h
    src/OO_Comp/config.h
)

add_opu_plugin(OO_Reverb
  SOURCES
    src/OO_Reverb/OO_Reverb.cpp
    src/OO_Reverb/OO_Reverb.h
    src/OO_Reverb/config.h
)

add_opu_plugin(OO_TrackPolish
  SOURCES
    src/OO_TrackPolish/OO_TrackPolish.cpp
    src/OO_TrackPolish/OO_TrackPolish.h
    src/OO_TrackPolish/config.h
)
