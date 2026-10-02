function(hax_set_warnings target)
  if(MSVC)
    # /utf-8: sources are UTF-8 and the injected UI scripts hold Turkish text;
    # without it MSVC decodes them with the system code page.
    target_compile_options(${target} PRIVATE /W4 /permissive- /EHsc /utf-8)
  else()
    target_compile_options(${target} PRIVATE
      -Wall -Wextra -Wpedantic -Wconversion -Wshadow
    )
  endif()
endfunction()
