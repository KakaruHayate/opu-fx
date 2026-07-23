cmake_minimum_required(VERSION 3.20)
project(OO_Reverb VERSION 0.1.0 LANGUAGES C CXX)

if(NOT DEFINED IPLUG2_DIR)
  set(IPLUG2_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../iPlug2" CACHE PATH "iPlug2 root")
endif()

include(${IPLUG2_DIR}/iPlug2.cmake)
find_package(iPlug2 REQUIRED)

target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../dsp)

iplug_add_plugin(${PROJECT_NAME}
  SOURCES OO_Reverb.cpp OO_Reverb.h config.h
  FORMATS VST3 CLAP APP
  LINK opufx_dsp
)
