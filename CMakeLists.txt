cmake_minimum_required(VERSION 3.20 FATAL_ERROR)

project(
  "OpenGL Engine"
  VERSION 0.0.1
  LANGUAGES CXX C
  )

# Include modules
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

# Colored compiler output
if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
  add_compile_options(-fdiagnostics-color=always)
elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
  add_compile_options(-fcolor-diagnostics)
endif()

include(warnings)
include(FeatureSummary)

add_subdirectory(external)
add_subdirectory(src)

feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES INCLUDE_QUIET_PACKAGES)
