add_executable(gpiotd gpiotd.c)
add_executable(gpiotc gpiotc.c)
add_executable(test_fx2 test_fx2.c)

target_include_directories(gpiotd PUBLIC "${PROJECT_SOURCE_DIR}/lib" PUBLIC ${GLIB_INCLUDE_DIRS} ${GIO_INCLUDE_DIRS})
target_link_libraries(gpiotd analyzer ${GLIB_LIBRARIES} ${GIO_LIBRARIES})

target_include_directories(gpiotc PUBLIC PUBLIC ${GLIB_INCLUDE_DIRS} ${GIO_INCLUDE_DIRS} "${PROJECT_SOURCE_DIR}/include")
target_link_libraries(gpiotc ${GLIB_LIBRARIES} ${GIO_LIBRARIES})

target_include_directories(test_fx2 PUBLIC "${PROJECT_SOURCE_DIR}/lib" PUBLIC ${GLIB_INCLUDE_DIRS} ${GIO_INCLUDE_DIRS})
target_link_libraries(test_fx2 analyzer ${GLIB_LIBRARIES} ${GIO_LIBRARIES})
