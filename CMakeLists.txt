cmake_minimum_required(VERSION 3.21)
project(ppr)

option(PPR_MIMALLOC "use mimalloc" OFF)

################################
# Basic Compiler Flags
################################

if(NOT DEFINED CMAKE_MSVC_RUNTIME_LIBRARY)
  if(PPR_MIMALLOC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
  else()
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
  endif()
endif()

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

if (NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Build type" FORCE)
endif ()
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")


################################
# Dependencies
################################
find_package(Threads)

if (MSVC)
  # PDB debug information is not supported by buildcache.
  # Store debug info in the object files.
  option(PPR_DEBUG_SYMBOLS "generate debug symbols (debug builds)" ON)
  if (PPR_DEBUG_SYMBOLS)
    set(PPR_MSVC_DEBUG_FLAGS "/Z7")
  else ()
    set(PPR_MSVC_DEBUG_FLAGS "")
  endif ()
  string(REPLACE "/Zi" "${PPR_MSVC_DEBUG_FLAGS}" CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG}")
  string(REPLACE "/Zi" "${PPR_MSVC_DEBUG_FLAGS}" CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}")
  string(REPLACE "/Zi" "${PPR_MSVC_DEBUG_FLAGS}" CMAKE_C_FLAGS_RELWITHDEBINFO "${CMAKE_C_FLAGS_RELWITHDEBINFO}")
  string(REPLACE "/Zi" "${PPR_MSVC_DEBUG_FLAGS}" CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO}")
endif ()

if(PPR_MIMALLOC)
  if(WIN32)
    set(MI_BUILD_SHARED ON)
  endif()
endif()

include(cmake/buildcache.cmake)
include(cmake/pkg.cmake)

if(PPR_MIMALLOC)
  if(WIN32)
    set(ppr-mimalloc-lib mimalloc)
    target_link_libraries(cista INTERFACE mimalloc)
    set(ppr-compile-definitions PPR_USE_MIMALLOC=1)
  else()
    set(ppr-mimalloc-lib mimalloc-obj)
    target_link_libraries(cista INTERFACE mimalloc-static)
  endif()
  target_compile_definitions(cista INTERFACE CISTA_USE_MIMALLOC=1)
  target_compile_definitions(boost INTERFACE BOOST_ASIO_DISABLE_STD_ALIGNED_ALLOC=1)
endif()

################################
# Linter
################################
option(PPR_LINT "Run clang-tidy with the compiler." OFF)
if(PPR_LINT)
  # clang-tidy will be run on all targets defined hereafter
  include(cmake/clang-tidy.cmake)
endif()


################################
# ppr-common library
################################
file(GLOB_RECURSE ppr-common-files
  src/common/*.cc
)
add_library(ppr-common ${ppr-common-files})
target_include_directories(ppr-common PUBLIC include)
target_link_libraries(ppr-common
  ${CMAKE_THREAD_LIBS_INIT}
  ${ppr-mimalloc-lib}
  boost
  utl
  cista
  unordered_dense
)
target_compile_features(ppr-common PUBLIC cxx_std_20)
set_target_properties(ppr-common PROPERTIES CXX_EXTENSIONS OFF)
target_compile_options(ppr-common PRIVATE ${ppr-compile-flags})
target_compile_definitions(ppr-common PRIVATE ${ppr-compile-definitions})

if(PPR_MIMALLOC AND WIN32 AND PROJECT_IS_TOP_LEVEL)
  add_custom_command(
    TARGET ppr-common POST_BUILD
    COMMAND "${CMAKE_COMMAND}" -E copy
      $<TARGET_FILE:mimalloc>
      $<TARGET_FILE_DIR:ppr-common>
    COMMENT "Copy mimalloc.dll to output directory"
  )
  add_custom_command(
    TARGET ppr-common POST_BUILD
    COMMAND "${CMAKE_COMMAND}" -E copy
      "${CMAKE_CURRENT_BINARY_DIR}/deps/mimalloc/mimalloc-redirect.dll"
      $<TARGET_FILE_DIR:ppr-common>
    COMMENT "Copy mimalloc-redirect.dll to output directory"
  )
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
target_link_libraries(ppr-routing
  ${CMAKE_THREAD_LIBS_INIT}
  ${ppr-mimalloc-lib}
  boost
  utl
  cista
  unordered_dense
  ppr-common
)
target_compile_features(ppr-routing PUBLIC cxx_std_20)
set_target_properties(ppr-routing PROPERTIES CXX_EXTENSIONS OFF)
target_compile_options(ppr-routing PRIVATE ${ppr-compile-flags})
target_compile_definitions(ppr-routing PRIVATE ${ppr-compile-definitions})


################################
# ppr-profiles library
################################
file(GLOB_RECURSE ppr-profiles-files
  src/profiles/*.cc
)
add_library(ppr-profiles ${ppr-profiles-files})
target_include_directories(ppr-profiles PUBLIC include)
target_link_libraries(ppr-profiles rapidjson)
target_compile_features(ppr-profiles PUBLIC cxx_std_20)
set_target_properties(ppr-profiles PROPERTIES CXX_EXTENSIONS OFF)
target_compile_options(ppr-profiles PRIVATE ${ppr-compile-flags})
target_compile_definitions(ppr-profiles PRIVATE ${ppr-compile-definitions})


################################
# ppr-preprocessing library
################################
file(GLOB_RECURSE ppr-preprocessing-files
  src/preprocessing/*.cc
  src/serialization/writer.cc
  src/serialization/reader.cc
)
add_library(ppr-preprocessing ${ppr-preprocessing-files})
target_include_directories(ppr-preprocessing PUBLIC include)
target_link_libraries(ppr-preprocessing
  boost-filesystem
  boost-iostreams
  ${CMAKE_THREAD_LIBS_INIT}
  ${ppr-mimalloc-lib}
  utl
  osmium
  protozero
  cista
  unordered_dense
  ppr-common
)
target_compile_features(ppr-preprocessing PUBLIC cxx_std_20)
set_target_properties(ppr-preprocessing PROPERTIES CXX_EXTENSIONS OFF)
target_compile_options(ppr-preprocessing PRIVATE ${ppr-compile-flags})
target_compile_definitions(ppr-preprocessing PRIVATE ${ppr-compile-definitions})


################################
# ppr-preprocess executable
################################
file(GLOB_RECURSE ppr-preprocess-files
  src/cmd/preprocess/*.cc
)
add_executable(ppr-preprocess ${ppr-preprocess-files})
target_include_directories(ppr-preprocess PUBLIC include)
target_link_libraries(ppr-preprocess
  ${CMAKE_THREAD_LIBS_INIT}
  ${ppr-mimalloc-lib}
  ppr-preprocessing
  ppr-common
  conf
)
target_compile_features(ppr-preprocess PUBLIC cxx_std_20)
set_target_properties(ppr-preprocess PROPERTIES CXX_EXTENSIONS OFF)
target_compile_options(ppr-preprocess PRIVATE ${ppr-compile-flags})
target_compile_definitions(ppr-preprocess PRIVATE ${ppr-compile-definitions})


################################
# ppr-backend executable
################################
file(GLOB_RECURSE ppr-backend-files
  src/cmd/backend/*.cc
  src/backend/*.cc
)
add_executable(ppr-backend ${ppr-backend-files})
target_link_libraries(ppr-backend
  ${CMAKE_THREAD_LIBS_INIT}
  ${ppr-mimalloc-lib}
  web-server-tls
  boost-filesystem
  boost-iostreams
  conf
  ppr-routing
  ppr-profiles
  ppr-common
  rapidjson
)
target_compile_features(ppr-backend PUBLIC cxx_std_20)
set_target_properties(ppr-backend PROPERTIES CXX_EXTENSIONS OFF)
target_compile_options(ppr-backend PRIVATE ${ppr-compile-flags})
target_compile_definitions(ppr-backend PRIVATE ${ppr-compile-definitions})


################################
# footrouting executable
################################
file(GLOB_RECURSE footrouting-files
  src/cmd/footrouting/*.cc
  src/backend/*.cc
)
add_executable(footrouting ${footrouting-files})
target_link_libraries(footrouting
  ${CMAKE_THREAD_LIBS_INIT}
  ${ppr-mimalloc-lib}
  web-server-tls
  boost-iostreams
  conf
  ppr-routing
  ppr-profiles
  ppr-preprocessing
  ppr-common
  osmium
  protozero
  rapidjson
)
target_compile_features(footrouting PUBLIC cxx_std_20)
set_target_properties(footrouting PROPERTIES CXX_EXTENSIONS OFF)
target_compile_options(footrouting PRIVATE ${ppr-compile-flags})
target_compile_definitions(footrouting PRIVATE ${ppr-compile-definitions})


################################
# ppr-benchmark executable
################################
file(GLOB_RECURSE ppr-benchmark-files
  src/cmd/benchmark/*.cc
  src/profiles/parse_search_profile.cc
)
add_executable(ppr-benchmark ${ppr-benchmark-files})
target_link_libraries(ppr-benchmark
  zlibstatic
  boost-filesystem
  ${CMAKE_THREAD_LIBS_INIT}
  ${ppr-mimalloc-lib}
  conf
  ppr-routing
  ppr-common
  cpptoml
  osmium
  protozero
  rapidjson
)
target_compile_features(ppr-benchmark PUBLIC cxx_std_20)
set_target_properties(ppr-benchmark PROPERTIES CXX_EXTENSIONS OFF)
target_compile_options(ppr-benchmark PRIVATE ${ppr-compile-flags})
target_compile_definitions(ppr-benchmark PRIVATE ${ppr-compile-definitions})


################################
# ppr-test executable
################################
enable_testing()
file(GLOB_RECURSE ppr-test-files
  test/*.cc
)
add_executable(ppr-test ${ppr-test-files})
target_link_libraries(ppr-test
  ${CMAKE_THREAD_LIBS_INIT}
  ${ppr-mimalloc-lib}
  gtest_main
  gmock
  ppr-common
  ppr-preprocessing
  ppr-routing
  ppr-profiles
)
target_compile_features(ppr-test PUBLIC cxx_std_20)
set_target_properties(ppr-test PROPERTIES CXX_EXTENSIONS OFF)
target_compile_options(ppr-test PRIVATE ${ppr-compile-flags})
target_compile_definitions(ppr-test PRIVATE ${ppr-compile-definitions})
include(GoogleTest)
gtest_discover_tests(ppr-test)


################################
# clang-format check
################################
find_program(CLANG_FORMAT_COMMAND NAMES clang-format clang-format-18)
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
