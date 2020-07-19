find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBSIGROK REQUIRED IMPORTED_TARGET libsigrok)


find_library(PIGPIO_LIBRARY
        NAMES libpigpio.so
        HINTS /usr/local/lib)

find_path(PIGPIO_INCLUDE_DIR
  NAMES pigpio.h)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)


add_library(analyzer collection/la_pigpio.c collection/la_sigrok.c output.c timestamp.c)

target_include_directories(analyzer
  PUBLIC ${LIBSIGROK_INCLUDE_DIRS}
  PUBLIC ${PIGPIO_INCLUDE_DIR}
  PUBLIC "${PROJECT_SOURCE_DIR}/include"
  )

target_link_libraries(analyzer
  ${LIBSIGROK_LIBRARIES}
  ${PIGPIO_LIBRARY}
  Threads::Threads
  )
