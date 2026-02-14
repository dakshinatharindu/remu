cmake_minimum_required(VERSION 3.20)

project(remu
  VERSION 0.1.0
  DESCRIPTION "RISC-V Emulator"
  LANGUAGES C CXX
)

# ---- Options ----
include(cmake/Options.cmake)

# ---- Global config ----
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Export compile_commands.json (nice for clangd)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(cmake/ToolchainWarnings.cmake)
include(cmake/Sanitizers.cmake)

# ---- Core library ----
add_library(remu_core)

target_include_directories(remu_core
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Collect sources (starter approach; later you can list explicitly if you prefer)
file(GLOB_RECURSE REMU_CORE_SOURCES CONFIGURE_DEPENDS
  ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/*.c
)

target_sources(remu_core PRIVATE ${REMU_CORE_SOURCES})

target_compile_features(remu_core PUBLIC cxx_std_20)

remu_set_warnings(remu_core)
remu_enable_sanitizers(remu_core)

# Common compile defs
target_compile_definitions(remu_core
  PUBLIC
    $<$<BOOL:${REMU_ENABLE_TRACE}>:REMU_ENABLE_TRACE=1>
    $<$<BOOL:${REMU_ENABLE_LOG}>:REMU_ENABLE_LOG=1>
)

# ---- Apps ----
add_subdirectory(apps/remu)

# ---- Tests ----
# if(REMU_BUILD_TESTS)
#   enable_testing()
#   add_subdirectory(tests)
# endif()
