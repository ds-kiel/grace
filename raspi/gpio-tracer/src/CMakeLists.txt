add_executable(gpiotd dbus-gpiotd.c)
add_executable(gpiotc dbus-gpiotc.c)

target_include_directories(gpiotd PUBLIC "${PROJECT_SOURCE_DIR}/lib" PUBLIC ${GLIB_INCLUDE_DIRS} ${GIO_INCLUDE_DIRS})
target_link_libraries(gpiotd analyzer ${GLIB_LIBRARIES} ${GIO_LIBRARIES})

target_include_directories(gpiotc PUBLIC PUBLIC ${GLIB_INCLUDE_DIRS} ${GIO_INCLUDE_DIRS} "${PROJECT_SOURCE_DIR}/include")
target_link_libraries(gpiotc ${GLIB_LIBRARIES} ${GIO_LIBRARIES})

# target_include_directories(gpiotd-static PUBLIC "${PROJECT_SOURCE_DIR}/lib" PUBLIC ${GLIB_INCLUDE_DIRS} ${GIO_INCLUDE_DIRS})
# target_link_libraries(gpiotd-static -static analyzer ${GLIB_LIBRARIES} ${GIO_LIBRARIES})

# add_executable(gpiotd-static dbus-gpiotd.c)
# add_executable(gpiotc-static dbus-gpiotc.c)

# ADD_LIBRARY(glib_static STATIC IMPORTED)
# SET_TARGET_PROPERTIES(glib_static PROPERTIES
#   IMPORTED_LOCATION /usr/lib/arm-linux-gnueabihf/libglib-2.0.a)

# ADD_LIBRARY(gio_static STATIC IMPORTED)
# SET_TARGET_PROPERTIES(gio_static PROPERTIES
#   IMPORTED_LOCATION /usr/lib/arm-linux-gnueabihf/libgio-2.0.a)o
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


# message("${GIO_LIBRARIES}")

# target_include_directories(gpiotc-static PUBLIC PUBLIC ${GLIB_INCLUDE_DIRS} ${GIO_INCLUDE_DIRS} "${PROJECT_SOURCE_DIR}/include")
# target_link_libraries(gpiotc-static -static gio_static gobject_static libffi_static
# libgmodule_static glib_static libmount_static libdl_static libblkid_static libz_static resolv_static
# selinux_static pcre_static uuid_static pthread )

# target_include_directories(gpiotd-static PUBLIC ${GLIB_INCLUDE_DIRS} ${GIO_INCLUDE_DIRS} "${PROJECT_SOURCE_DIR}/include")
# target_link_libraries(gpiotd-static analyzer-static ${GLIB_LIBRARIES} ${GIO_LIBRARIES})
