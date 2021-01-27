cmake_minimum_required(VERSION 3.14 FATAL_ERROR)

project(tidre-demo VERSION 0.0.1 LANGUAGES CXX)

find_package(Arrow 1.0.1 CONFIG REQUIRED)

find_library(FLETCHER fletcher)
if(NOT FLETCHER)
  message(FATAL_ERROR "Fletcher runtime library was not found")
endif()
add_library(fletcher SHARED IMPORTED GLOBAL)
set_target_properties(fletcher PROPERTIES IMPORTED_LOCATION ${FLETCHER})

include(FetchContent)

# CMake Modules
FetchContent_Declare(cmake-modules
  GIT_REPOSITORY  https://github.com/abs-tudelft/cmake-modules.git
  GIT_TAG         master
  )
FetchContent_MakeAvailable(cmake-modules)
include(CompileUnits)

add_compile_unit(
  NAME tidre-demo
  TYPE EXECUTABLE
  PRPS
    CXX_STANDARD 11
    CXX_STANDARD_REQUIRED ON
  SRCS
    src/main.cpp
  DEPS
    arrow_shared
    fletcher
)

compile_units()
