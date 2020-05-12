cmake_minimum_required(VERSION 3.16)
project(ppr)

################################
# Basic Compiler Flags
################################
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

if(MSVC)
  set(ppr-compile-flags
    /W3
    /DNOMINMAX
  )
else()
  set(ppr-compile-flags
    -Wall
    -Wextra
    -Wno-unknown-pragmas
    -pedantic
  )
endif()

if(APPLE)
  # prevents ar from invoking ranlib, let CMake do it
  set(CMAKE_C_ARCHIVE_CREATE "<CMAKE_AR> Scr <TARGET> <LINK_FLAGS> <OBJECTS>")
  set(CMAKE_CXX_ARCHIVE_CREATE "<CMAKE_AR> Scr <TARGET> <LINK_FLAGS> <OBJECTS>")

  # tell ranlib to ignore empty compilation units
  set(CMAKE_C_ARCHIVE_FINISH "<CMAKE_RANLIB> -no_warning_for_no_symbols -c <TARGET>")
  set(CMAKE_CXX_ARCHIVE_FINISH "<CMAKE_RANLIB> -no_warning_for_no_symbols -c <TARGET>")
endif()

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE "RelWithDebInfo" CACHE STRING "Build type" FORCE)
endif()
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")


################################
# Dependencies
################################
if (MSVC)
  set(Boost_USE_STATIC_LIBS ON)
  set(Boost_USE_MULTITHREADED ON)
  set(Boost_USE_STATIC_RUNTIME ON)
endif()
find_package(Boost REQUIRED system filesystem thread program_options iostreams)
find_package(Threads)

include(cmake/pkg.cmake)


################################
# Linter
################################
option(PPR_LINT "Run clang-tidy with the compiler." OFF)
if(PPR_LINT)
  # clang-tidy will be run on all targets defined hereafter
  include(cmake/clang-tidy.cmake)
endif()


################################
# ppr-routing library
################################
file(GLOB_RECURSE ppr-routing-files
  src/routing/*.cc
  src/serialization/reader.cc
)
add_library(ppr-routing ${ppr-routing-files})
target_include_directories(ppr-routing PUBLIC include)
target_include_directories(ppr-routing SYSTEM PUBLIC ${Boost_INCLUDE_DIRS})
target_link_libraries(ppr-routing
  ${CMAKE_THREAD_LIBS_INIT}
  ${Boost_LIBRARIES}
  utl
  cista
)
target_compile_features(ppr-routing PUBLIC cxx_std_17)
set_target_properties(ppr-routing PROPERTIES CXX_EXTENSIONS OFF)
target_compile_options(ppr-routing PRIVATE ${ppr-compile-flags})


################################
# ppr-profiles library
################################
file(GLOB_RECURSE ppr-profiles-files
  src/profiles/*.cc
)
add_library(ppr-profiles ${ppr-profiles-files})
target_include_directories(ppr-profiles PUBLIC include)
target_include_directories(ppr-profiles SYSTEM PUBLIC ${Boost_INCLUDE_DIRS})
target_link_libraries(ppr-profiles rapidjson)
target_compile_features(ppr-profiles PUBLIC cxx_std_17)
set_target_properties(ppr-profiles PROPERTIES CXX_EXTENSIONS OFF)
target_compile_options(ppr-profiles PRIVATE ${ppr-compile-flags})


################################
# ppr-preprocessing executable
################################
file(GLOB_RECURSE ppr-preprocessing-files
  src/preprocessing/*.cc
  src/serialization/writer.cc
  src/serialization/reader.cc
)
add_library(ppr-preprocessing ${ppr-preprocessing-files})
target_include_directories(ppr-preprocessing PUBLIC include)
#target_include_directories(ppr-preprocessing PUBLIC ${CMAKE_BINARY_DIR}/generated)
target_include_directories(ppr-preprocessing SYSTEM PUBLIC ${Boost_INCLUDE_DIRS})
target_link_libraries(ppr-preprocessing
  zlibstatic
  ${CMAKE_THREAD_LIBS_INIT}
  ${Boost_LIBRARIES}
  utl
  osmium
  protozero
  cista
)
target_compile_features(ppr-preprocessing PUBLIC cxx_std_17)
set_target_properties(ppr-preprocessing PROPERTIES CXX_EXTENSIONS OFF)
target_compile_options(ppr-preprocessing PRIVATE ${ppr-compile-flags})


################################
# ppr-preprocess executable
################################
file(GLOB_RECURSE ppr-preprocess-files
  src/cmd/preprocess/*.cc
)
add_executable(ppr-preprocess ${ppr-preprocess-files})
target_include_directories(ppr-preprocess PUBLIC include)
target_include_directories(ppr-preprocess SYSTEM PUBLIC ${Boost_INCLUDE_DIRS})
target_link_libraries(ppr-preprocess
  ppr-preprocessing
  conf
)
target_compile_features(ppr-preprocess PUBLIC cxx_std_17)
set_target_properties(ppr-preprocess PROPERTIES CXX_EXTENSIONS OFF)
target_compile_options(ppr-preprocess PRIVATE ${ppr-compile-flags})


################################
# ppr-backend executable
################################
file(GLOB_RECURSE ppr-backend-files
  src/cmd/backend/*.cc
  src/backend/*.cc
)
add_executable(ppr-backend ${ppr-backend-files})
target_include_directories(ppr-backend SYSTEM PUBLIC ${Boost_INCLUDE_DIRS})
target_link_libraries(ppr-backend
  zlibstatic
  ${CMAKE_THREAD_LIBS_INIT}
  ${Boost_LIBRARIES}
  web-server-tls
  conf
  ppr-routing
  ppr-profiles
  rapidjson
)
target_compile_features(ppr-backend PUBLIC cxx_std_17)
set_target_properties(ppr-backend PROPERTIES CXX_EXTENSIONS OFF)
target_compile_options(ppr-backend PRIVATE ${ppr-compile-flags})


################################
# footrouting executable
################################
file(GLOB_RECURSE footrouting-files
  src/cmd/footrouting/*.cc
  src/backend/*.cc
)
add_executable(footrouting ${footrouting-files})
target_include_directories(footrouting SYSTEM PUBLIC ${Boost_INCLUDE_DIRS})
target_link_libraries(footrouting
  zlibstatic
  ${CMAKE_THREAD_LIBS_INIT}
  ${Boost_LIBRARIES}
  web-server-tls
  conf
  ppr-routing
  ppr-profiles
  ppr-preprocessing
  osmium
  protozero
  rapidjson
)
target_compile_features(footrouting PUBLIC cxx_std_17)
set_target_properties(footrouting PROPERTIES CXX_EXTENSIONS OFF)
target_compile_options(footrouting PRIVATE ${ppr-compile-flags})


################################
# ppr-benchmark executable
################################
file(GLOB_RECURSE ppr-benchmark-files
  src/cmd/benchmark/*.cc
  src/profiles/parse_search_profile.cc
)
add_executable(ppr-benchmark ${ppr-benchmark-files})
target_include_directories(ppr-benchmark SYSTEM PUBLIC ${Boost_INCLUDE_DIRS})
target_link_libraries(ppr-benchmark
  zlibstatic
  ${CMAKE_THREAD_LIBS_INIT}
  ${Boost_LIBRARIES}
  conf
  ppr-routing
  cpptoml
  osmium
  protozero
  rapidjson
)
target_compile_features(ppr-benchmark PUBLIC cxx_std_17)
set_target_properties(ppr-benchmark PROPERTIES CXX_EXTENSIONS OFF)
target_compile_options(ppr-benchmark PRIVATE ${ppr-compile-flags})


################################
# clang-format check
################################
find_program(CLANG_FORMAT_COMMAND NAMES clang-format-9 clang-format-8 clang-format)
add_custom_target(ppr-format-check
  COMMAND find
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    -type f
    (
    -name "*.cc"
    -o
    -name "*.h"
    )
    -print0
    | xargs -0 ${CLANG_FORMAT_COMMAND} -i
  COMMAND git status --porcelain
  COMMAND git status --porcelain | xargs -I {} -0 test -z \"{}\"
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  COMMENT "Checking source code formatting"
  VERBATIM
)
