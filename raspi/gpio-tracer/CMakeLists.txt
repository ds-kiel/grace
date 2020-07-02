cmake_minimum_required(VERSION 3.13)

project(logic-analyzer)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(PkgConfig REQUIRED)
pkg_search_module(GLIB REQUIRED glib-2.0)
pkg_search_module(GIO REQUIRED gio-2.0)

add_subdirectory(lib)
add_subdirectory(src)
