cmake_minimum_required(VERSION 3.14)

project("Sentry Toy Project" LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Able to manually set the sentry-native installation directory
set(SENTRY_DIR Default CACHE STRING "Path to sentry-native lib and include")
if(SENTRY_DIR STREQUAL "Default")
    set(SENTRY_DIR ${PROJECT_SOURCE_DIR}/../../apps/sentry-native/install)
    message(STATUS "SENTRY_DIR not set, using default of ${SENTRY_DIR}")
else()
    message(STATUS "SENTRY_DIR set to ${SENTRY_DIR}")
endif()
set(SENTRY_LIB_PATH ${SENTRY_DIR}/lib)
set(SENTRY_INCLUDE_DIR ${SENTRY_DIR}/include)

add_executable(main main.cpp)
target_include_directories(main PRIVATE ${SENTRY_INCLUDE_DIR})
target_link_directories(main PRIVATE ${SENTRY_LIB_PATH})
target_link_libraries(main PRIVATE sentry)
