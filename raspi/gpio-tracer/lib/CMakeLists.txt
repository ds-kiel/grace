find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBSIGROK REQUIRED IMPORTED_TARGET libsigrok)


set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

set(CMAKE_INSTALL_RPATH "$ORIGIN")

add_library(analyzer collection/preprocess.c output.c timestamp.c transmitter.c)

target_include_directories(analyzer
  PUBLIC ${LIBSIGROK_INCLUDE_DIRS}
  PUBLIC ${GLIB_INCLUDE_DIRS}
  PUBLIC "${PROJECT_SOURCE_DIR}/include"
  )

target_link_libraries(analyzer
  ${LIBSIGROK_LIBRARIES}
  ${GLIB_LIBRARIES}
  Threads::Threads
  )
