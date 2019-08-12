cmake_minimum_required(VERSION 3.8)

# C++ Standard
if(NOT "${CMAKE_CXX_STANDARD}")
	set(CMAKE_CXX_STANDARD 17)
endif()
set(CMAKE_CXX_EXTENSIONS OFF)

# Release Build
if (NOT EXISTS ${CMAKE_BINARY_DIR}/CMakeCache.txt)
   if (NOT CMAKE_BUILD_TYPE)
	  set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "" FORCE)
   endif()
endif()

# Project
project(dbus-mockery)

# Add Executable
add_library(${PROJECT_NAME} STATIC
   "source/dbus-mockery/bindings/message.cpp"
   "source/dbus-mockery/bindings/types.cpp"
   "source/dbus-mockery/bindings/bus.cpp"
   "source/dbus-mockery/bindings/object_path.cpp"
   "source/dbus-mockery/bindings/signature.cpp"
   "source/dbus-mockery/generator/generator.cpp"
   "source/dbus-mockery/generator/introspect.cpp"
   "source/dbus-mockery/bindings/slot.cpp"
   "source/dbus-mockery/bindings/event_loop.cpp"
   "source/dbus-mockery/bindings/busy_loop.cpp"
   "source/dbus-mockery/bindings/detail/slot_holder.cpp"
)

# Include Dirs
target_include_directories(${PROJECT_NAME} PUBLIC "include" "../dbus-mockery-system/include")

include(FindPkgConfig)

pkg_check_modules(SYSTEMD "libsystemd")

message(STATUS "${SYSTEMD}")

target_link_libraries(${PROJECT_NAME} PUBLIC
   ${SYSTEMD_LIBRARIES}
   -lpthread
)

# Compiler Options
set(DEBUG_OPTIONS -fexceptions -g -Wall -pedantic-errors -pedantic)
set(RELEASE_OPTIONS -fexceptions -O3 -Wall -pedantic-errors -pedantic)
target_compile_options(${PROJECT_NAME} PUBLIC "$<$<CONFIG:DEBUG>:${DEBUG_OPTIONS}>")
target_compile_options(${PROJECT_NAME} PUBLIC "$<$<CONFIG:RELEASE>:${RELEASE_OPTIONS}>")

# Install
install(TARGETS dbus-mockery DESTINATION lib)
install(DIRECTORY ${CMAKE_SOURCE_DIR}/include/ DESTINATION include FILES_MATCHING PATTERN "*.h*")