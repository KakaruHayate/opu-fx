cmake_minimum_required(VERSION 3.20)
project(opu-fx VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT DEFINED IPLUG2_DIR)
  set(IPLUG2_DIR "${CMAKE_CURRENT_SOURCE_DIR}/iPlug2" CACHE PATH "iPlug2 root")
endif()

add_subdirectory(src/OO_EQ)
add_subdirectory(src/OO_Comp)
add_subdirectory(src/OO_Reverb)
add_subdirectory(src/OO_TrackPolish)
