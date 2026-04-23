cmake_minimum_required(VERSION 3.16.0)
project(Radar VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

file(GLOB_RECURSE SRC src/*.cpp thirdparty/tinyxml2/tinyxml2.cpp)

set(CMAKE_PREFIX_PATH "C:/msys64/ucrt64")

add_subdirectory(thirdparty/SDL)
add_subdirectory(thirdparty/bxzstr)

# Set these BEFORE FetchContent pulls anything
set(CPR_USE_SYSTEM_CURL ON)
set(CPR_FORCE_USE_SYSTEM_CURL ON)

include(FetchContent)

# Declare and populate cpr first without adding subdirectory
FetchContent_Declare(cpr
    GIT_REPOSITORY https://github.com/libcpr/cpr.git
    GIT_TAG 1.14.2)

# Fetch cpr source so it can declare curl internally
FetchContent_Populate(cpr)

# Now force curl options before cpr's CMakeLists runs
set(CURL_USE_LIBPSL      OFF CACHE BOOL "" FORCE)
set(CURL_USE_LIBIDN2     OFF CACHE BOOL "" FORCE)

# Now add cpr's subdirectory (which will trigger curl's FetchContent)
add_subdirectory(${cpr_SOURCE_DIR} ${cpr_BINARY_DIR})

add_executable(Radar ${SRC})
target_link_libraries(Radar PRIVATE cpr::cpr bxzstr::bxzstr)
if (CMAKE_BUILD_TYPE STREQUAL "Release")
    target_compile_options(Radar PRIVATE -O3 -flto -march=native)
    target_link_options(Radar PRIVATE -flto)
else()
    target_compile_options(Radar PRIVATE -g)
endif()
target_include_directories(Radar PRIVATE thirdparty/tinyxml2)