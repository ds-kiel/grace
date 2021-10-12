find_package(PkgConfig REQUIRED)
# pkg_check_modules(LIBSIGROK REQUIRED IMPORTED_TARGET libsigrok)
pkg_check_modules(LIBUSB REQUIRED IMPORTED_TARGET libusb-1.0)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

set(CMAKE_INSTALL_RPATH "$ORIGIN")

set(ANALYZER_SOURCES tracing.c chunked_output.c radio.c fx2.c)

if(WITH_RADIO_SYNC)
  set(ANALYZER_SOURCES ${ANALYZER_SOURCES} "${PROJECT_SOURCE_DIR}/external/cc1101-spidev/cc1101.c")
endif(WITH_RADIO_SYNC)

add_library(analyzer ${ANALYZER_SOURCES})

target_include_directories(analyzer
  PUBLIC ${GLIB_INCLUDE_DIRS}
  PUBLIC ${LIBUSB_INCLUDE_DIRS}
  PUBLIC ${PROJECT_SOURCE_DIR}/include
  PUBLIC "${PROJECT_SOURCE_DIR}/external"
  )

target_link_libraries(analyzer
  ${GLIB_LIBRARIES}
  ${LIBUSB_LIBRARIES}
  Threads::Threads
  )

