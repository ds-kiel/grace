add_executable(logic-analyzer main.c)
target_include_directories(logic-analyzer PUBLIC "${PROJECT_SOURCE_DIR}/lib")
target_link_libraries(logic-analyzer analyzer)
