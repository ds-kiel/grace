add_executable(gpiotd gpiotd.c)
add_executable(gpiotc gpiotc.c)
add_executable(test_fx2 test_fx2.c)
add_executable(test_tracing test_tracing.c)
add_executable(load-firmware load-firmware.c)
add_executable(write-eeprom write-eeprom.c)

target_include_directories(gpiotd PUBLIC ${GLIB_INCLUDE_DIRS} PUBLIC "${PROJECT_SOURCE_DIR}/lib"  ${GIO_INCLUDE_DIRS})
target_link_libraries(gpiotd analyzer ${GLIB_LIBRARIES} ${GIO_LIBRARIES})

target_include_directories(gpiotc PUBLIC PUBLIC ${GLIB_INCLUDE_DIRS} ${GIO_INCLUDE_DIRS} "${PROJECT_SOURCE_DIR}/include")
target_link_libraries(gpiotc ${GLIB_LIBRARIES} ${GIO_LIBRARIES})

target_include_directories(test_fx2 PUBLIC "${PROJECT_SOURCE_DIR}/lib" PUBLIC ${GLIB_INCLUDE_DIRS} ${GIO_INCLUDE_DIRS})
target_link_libraries(test_fx2 analyzer ${GLIB_LIBRARIES} ${GIO_LIBRARIES})

target_include_directories(test_tracing PUBLIC "${PROJECT_SOURCE_DIR}/lib" PUBLIC ${GLIB_INCLUDE_DIRS} ${GIO_INCLUDE_DIRS})
target_link_libraries(test_tracing analyzer ${GLIB_LIBRARIES} ${GIO_LIBRARIES})

target_include_directories(load-firmware PUBLIC "${PROJECT_SOURCE_DIR}/lib" PUBLIC ${GLIB_INCLUDE_DIRS} ${GIO_INCLUDE_DIRS})
target_link_libraries(load-firmware analyzer ${GLIB_LIBRARIES} ${GIO_LIBRARIES})

target_include_directories(write-eeprom PUBLIC "${PROJECT_SOURCE_DIR}/lib" PUBLIC ${GLIB_INCLUDE_DIRS} ${GIO_INCLUDE_DIRS})
target_link_libraries(write-eeprom analyzer ${GLIB_LIBRARIES} ${GIO_LIBRARIES})
