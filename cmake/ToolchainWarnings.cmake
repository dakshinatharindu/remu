function(remu_set_warnings target)
  if(MSVC)
    target_compile_options(${target} PRIVATE
      /W4
      /permissive-
      /Zc:__cplusplus
    )
  else()
    target_compile_options(${target} PRIVATE
      -Wall -Wextra -Wpedantic
      -Wshadow
      -Wconversion
      -Wsign-conversion
      -Wold-style-cast
      -Woverloaded-virtual
      -Wnull-dereference
    )
  endif()
endfunction()
