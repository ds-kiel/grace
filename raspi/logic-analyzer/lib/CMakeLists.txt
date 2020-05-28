find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBSIGROK REQUIRED IMPORTED_TARGET libsigrok)

add_library(analyzer context.c output.c)

target_include_directories(analyzer
  PUBLIC ${LIBSIGROK_INCLUDE_DIRS})

target_link_libraries(analyzer
  ${LIBSIGROK_LIBRARIES}
  )
