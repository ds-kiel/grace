find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBSIGROK REQUIRED IMPORTED_TARGET libsigrok)

find_library(PIGPIO_LIBRARY
        NAMES libpigpio.so
        HINTS /usr/local/lib)

find_path(PIGPIO_INCLUDE_DIR
  NAMES pigpio.h)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

set(CMAKE_INSTALL_RPATH "$ORIGIN")

add_library(analyzer collection/la_sigrok.c output.c timestamp.c)
add_library(transmitter SHARED transmitter.c)


target_include_directories(analyzer
  PUBLIC ${LIBSIGROK_INCLUDE_DIRS}
  PUBLIC "${PROJECT_SOURCE_DIR}/include"
  )

target_link_libraries(analyzer
  ${LIBSIGROK_LIBRARIES}
  Threads::Threads
  )

target_include_directories(transmitter
  PUBLIC ${PIGPIO_INCLUDE_DIR}
  PUBLIC ${GLIB_INCLUDE_DIRS}
  PUBLIC "${PROJECT_SOURCE_DIR}/include"
  )

target_link_libraries(transmitter
  ${GLIB_LIBRARIES}
  ${PIGPIO_LIBRARY})
