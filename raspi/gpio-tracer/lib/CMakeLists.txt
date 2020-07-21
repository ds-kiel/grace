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

add_library(transmitter SHARED transmitter.c)

target_include_directories(analyzer
  PUBLIC ${LIBSIGROK_INCLUDE_DIRS}
  PUBLIC "${PROJECT_SOURCE_DIR}/include"
  )

target_link_libraries(analyzer
  ${LIBSIGROK_LIBRARIES}
  Threads::Threads
  )

# target_include_directories(analyzer-static
#   PUBLIC ${LIBSIGROK_INCLUDE_DIRS}
#   PUBLIC ${PIGPIO_INCLUDE_DIR}
#   PUBLIC "${PROJECT_SOURCE_DIR}/include"
#   )

# add_library(analyzer-static SHARED collection/la_pigpio.c collection/la_sigrok.c output.c timestamp.c)
# ADD_LIBRARY(sigrok_static SHARED IMPORTED)
# SET_TARGET_PROPERTIES(sigrok_static PROPERTIES
#   IMPORTED_LOCATION /usr/lib/arm-linux-gnueabihf/libsigrok.a)

# ADD_LIBRARY(glib_static STATIC IMPORTED)
# SET_TARGET_PROPERTIES(glib_static PROPERTIES
#   IMPORTED_LOCATION /usr/lib/arm-linux-gnueabihf/libglib-2.0.a)

# ADD_LIBRARY(gio_static STATIC IMPORTED)
# SET_TARGET_PROPERTIES(gio_static PROPERTIES
#   IMPORTED_LOCATION /usr/lib/arm-linux-gnueabihf/libgio-2.0.a)

# ADD_LIBRARY(pcre_static STATIC IMPORTED)
# SET_TARGET_PROPERTIES(pcre_static PROPERTIES
#   IMPORTED_LOCATION /usr/lib/arm-linux-gnueabihf/libpcre.a)

# ADD_LIBRARY(gobject_static STATIC IMPORTED)
# SET_TARGET_PROPERTIES(gobject_static PROPERTIES
#   IMPORTED_LOCATION /usr/lib/arm-linux-gnueabihf/libgobject-2.0.a)

# ADD_LIBRARY(libffi_static STATIC IMPORTED)
# SET_TARGET_PROPERTIES(libffi_static PROPERTIES
#   IMPORTED_LOCATION /usr/lib/arm-linux-gnueabihf/libffi.a)

# ADD_LIBRARY(selinux_static STATIC IMPORTED)
# SET_TARGET_PROPERTIES(selinux_static PROPERTIES
#   IMPORTED_LOCATION /usr/lib/arm-linux-gnueabihf/libselinux.a)

# ADD_LIBRARY(libmount_static STATIC IMPORTED)
# SET_TARGET_PROPERTIES(libmount_static PROPERTIES
#   IMPORTED_LOCATION /usr/lib/arm-linux-gnueabihf/libmount.a)

# ADD_LIBRARY(libblkid_static STATIC IMPORTED)
# SET_TARGET_PROPERTIES(libblkid_static PROPERTIES
#   IMPORTED_LOCATION /usr/lib/arm-linux-gnueabihf/libblkid.a)

# ADD_LIBRARY(libz_static STATIC IMPORTED)
# SET_TARGET_PROPERTIES(libz_static PROPERTIES
#   IMPORTED_LOCATION /usr/lib/arm-linux-gnueabihf/libz.a)

# ADD_LIBRARY(libgmodule_static STATIC IMPORTED)
# SET_TARGET_PROPERTIES(libgmodule_static PROPERTIES
#   IMPORTED_LOCATION /usr/lib/arm-linux-gnueabihf/libgmodule-2.0.a)

# ADD_LIBRARY(libdl_static STATIC IMPORTED)
# SET_TARGET_PROPERTIES(libdl_static PROPERTIES
#   IMPORTED_LOCATION /usr/lib/arm-linux-gnueabihf/libdl.a)

# ADD_LIBRARY(resolv_static STATIC IMPORTED)
# SET_TARGET_PROPERTIES(resolv_static PROPERTIES
#   IMPORTED_LOCATION /usr/lib/arm-linux-gnueabihf/libresolv.a)

# ADD_LIBRARY(uuid_static STATIC IMPORTED)
# SET_TARGET_PROPERTIES(uuid_static PROPERTIES
#   IMPORTED_LOCATION /usr/lib/arm-linux-gnueabihf/libuuid.a)

# ADD_LIBRARY(libusb_static STATIC IMPORTED)
# SET_TARGET_PROPERTIES(libusb_static PROPERTIES
#   IMPORTED_LOCATION /usr/lib/arm-linux-gnueabihf/libusb-1.0.a)

# target_link_libraries(analyzer-static
#   -static
#   sigrok_static
#   -shared
#   ${GIO_LIBRARIES}
#   ${GLIB_LIBARIES}
#   # libusb_static
#   # gio_static gobject_static libffi_static
#   # libgmodule_static glib_static libmount_static libdl_static libblkid_static libz_static resolv_static
#   # selinux_static pcre_static uuid_static pthread

#   Threads::Threads
#   )

target_include_directories(transmitter
  PUBLIC ${PIGPIO_INCLUDE_DIR}
  PUBLIC ${GLIB_INCLUDE_DIRS}
  PUBLIC "${PROJECT_SOURCE_DIR}/include"
  )

target_link_libraries(transmitter
  ${GLIB_LIBRARIES}
  ${PIGPIO_LIBRARY})
