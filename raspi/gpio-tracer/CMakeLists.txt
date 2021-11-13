cmake_minimum_required(VERSION 3.13)

project(logic-analyzer)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(PkgConfig REQUIRED)
pkg_search_module(GLIB REQUIRED glib-2.0)
pkg_search_module(GIO REQUIRED gio-2.0)

option(WITH_RADIO_SYNC "Designate one channel for external time synchronization signal" ON)

if(WITH_RADIO_SYNC)
  add_compile_definitions(WITH_TIMESYNC)
endif(WITH_RADIO_SYNC)

add_subdirectory(lib)
add_subdirectory(src)
