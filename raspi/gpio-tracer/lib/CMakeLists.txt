find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBSIGROK REQUIRED IMPORTED_TARGET libsigrok)


set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

set(CMAKE_INSTALL_RPATH "$ORIGIN")

add_library(analyzer
  tracing.c chunked_output.c radio.c
  "${PROJECT_SOURCE_DIR}/external/cc1101-spidev/cc1101.c")

target_include_directories(analyzer
  PUBLIC ${LIBSIGROK_INCLUDE_DIRS}
  PUBLIC ${GLIB_INCLUDE_DIRS}
  PUBLIC "${PROJECT_SOURCE_DIR}/include"
  PUBLIC "${PROJECT_SOURCE_DIR}/external"
  )

target_link_libraries(analyzer
  ${LIBSIGROK_LIBRARIES}
  ${GLIB_LIBRARIES}
  Threads::Threads
  )
