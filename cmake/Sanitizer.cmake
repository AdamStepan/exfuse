if(NOT "${CMAKE_C_COMPILER_ID}" MATCHES "[Cc]lang")
    message(FATAL_ERROR "Sanitizer is currently supported only by clang")
endif()

add_compile_options(-fsanitize=address -fno-omit-frame-pointer)
link_libraries(-fsanitize=address)
