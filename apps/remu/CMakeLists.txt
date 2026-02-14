add_executable(remu
  main.cpp
)

target_link_libraries(remu
  PRIVATE
    remu_core
)

remu_set_warnings(remu)
remu_enable_sanitizers(remu)

# Put the binary in a predictable place
set_target_properties(remu PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)
