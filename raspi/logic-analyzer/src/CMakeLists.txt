add_executable(gpiotd gpiotd.c)
add_executable(gpiotc gpiotc.c)

target_include_directories(gpiotd PUBLIC "${PROJECT_SOURCE_DIR}/lib")
target_link_libraries(gpiotd analyzer)
