cmake_minimum_required(VERSION 3.14 FATAL_ERROR)

project(tidre-demo VERSION 0.1.0 LANGUAGES CXX)

find_package(Arrow 3.0.0 CONFIG REQUIRED)

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

execute_process (
    COMMAND bash -c "awk -F= '/^ID=/{print $2}' /etc/os-release |tr -d '\n' | tr -d '\"'"
    OUTPUT_VARIABLE OS_NAME
)

execute_process (
    COMMAND bash -c "awk -F= '/^VERSION_ID=/{print $2}' /etc/os-release |tr -d '\n' | tr -d '\"'"
    OUTPUT_VARIABLE OS_VERSION
)

if(OS_NAME MATCHES "ubuntu")
  set(CPACK_DEBIAN_PACKAGE_RELEASE "ubuntu${OS_VERSION}")
  set(CPACK_GENERATOR "DEB")
elseif(OS_NAME MATCHES "centos")
  set(CPACK_RPM_PACKAGE_RELEASE_DIST "el${OS_VERSION}")
  if(OS_VERSION MATCHES "7")
    set(CPACK_GENERATOR "RPM;TGZ")
  else()
    set(CPACK_GENERATOR "RPM")
  endif()
endif()

set(CPACK_PACKAGE_VENDOR "Teratide")
set(CPACK_PACKAGE_VERSION_MAJOR "${tidre-demo_VERSION_MAJOR}")
set(CPACK_PACKAGE_VERSION_MINOR "${tidre-demo_VERSION_MINOR}")
set(CPACK_PACKAGE_VERSION_PATCH "${tidre-demo_VERSION_PATCH}")
set(CPACK_PACKAGE_RELOCATABLE ON)

set(CPACK_DEBIAN_PACKAGE_NAME "${CMAKE_PROJECT_NAME}")
set(CPACK_DEBIAN_FILE_NAME "DEB-DEFAULT")
set(CPACK_DEBIAN_PACKAGE_DEPENDS "fletcher = 0.0.19")
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "${CPACK_PACKAGE_VENDOR}")

set(CPACK_RPM_FILE_NAME "RPM-DEFAULT")
set(CPACK_RPM_PACKAGE_DESCRIPTION "${CPACK_PACKAGE_DESCRIPTION}")
set(CPACK_RPM_PACKAGE_LICENSE "ASL 2.0")
set(CPACK_RPM_PACKAGE_REQUIRES "fletcher = 0.0.19")

set(CPACK_ARCHIVE_FILE_NAME "${CMAKE_PROJECT_NAME}-${tidre-demo_VERSION}-${CMAKE_SYSTEM_NAME}")

include(CPack)
