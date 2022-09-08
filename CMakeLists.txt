cmake_minimum_required(VERSION 3.8)

# Project
project(dbus-glue)

# Add Executable
add_library(
  dbus-glue STATIC
  "source/dbus-glue/bindings/message.cpp"
  "source/dbus-glue/bindings/types.cpp"
  "source/dbus-glue/bindings/bus.cpp"
  "source/dbus-glue/bindings/object_path.cpp"
  "source/dbus-glue/bindings/signature.cpp"
  "source/dbus-glue/generator/generator.cpp"
  "source/dbus-glue/generator/introspect.cpp"
  "source/dbus-glue/bindings/slot.cpp"
  "source/dbus-glue/bindings/event_loop.cpp"
  "source/dbus-glue/bindings/busy_loop.cpp"
  "source/dbus-glue/bindings/detail/slot_holder.cpp"
  "source/dbus-glue/bindings/detail/bus_error.c"
  "source/dbus-glue/bindings/exposables/exposable_method.cpp"
  "source/dbus-glue/bindings/exposables/exposable_property.cpp")

include(FindPkgConfig)

pkg_check_modules(SYSTEMD "libsystemd")

message(STATUS "${SYSTEMD}")

target_link_libraries(dbus-glue PUBLIC ${SYSTEMD_LIBRARIES} -lpthread)
target_include_directories(dbus-glue
                           PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")

# Compiler Options
set(DEBUG_OPTIONS -fexceptions -g -Wall -pedantic-errors -pedantic)
set(RELEASE_OPTIONS -fexceptions -O3)
target_compile_options(dbus-glue PUBLIC "$<$<CONFIG:DEBUG>:${DEBUG_OPTIONS}>")
target_compile_options(dbus-glue
                       PUBLIC "$<$<CONFIG:RELEASE>:${RELEASE_OPTIONS}>")
set_target_properties(dbus-glue PROPERTIES CXX_STANDARD 20)

# Install
install(TARGETS dbus-glue DESTINATION lib)
install(
  DIRECTORY ${CMAKE_SOURCE_DIR}/include/
  DESTINATION include
  FILES_MATCHING
  PATTERN "*.h*")
