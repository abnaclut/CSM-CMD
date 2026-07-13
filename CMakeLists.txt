cmake_minimum_required(VERSION 3.16)
project(csm_cmd LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

option(BUILD_TERMINAL_TESTS "Build csm_cmd unit tests" ON)

include(FetchContent)

# replxx
FetchContent_Declare(
        replxx
        GIT_REPOSITORY https://github.com/AmokHuginnsson/replxx.git
        GIT_TAG        release-0.0.4
)

set(REPLXX_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(replxx)

add_library(csm_cmd STATIC
        src/CmdParser/command_parser.cpp
        src/CmdReg/command_registry.cpp
        src/CsmTerminal/csm_terminal.cpp
        src/Logger/logger.cpp
)

target_include_directories(csm_cmd
        PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(csm_cmd PUBLIC replxx)

target_compile_options(csm_cmd PRIVATE -Wall -Wextra -Wpedantic)

find_package(Threads REQUIRED)
target_link_libraries(csm_cmd PUBLIC Threads::Threads)

add_executable(csm_terminal src/main.cpp)
target_link_libraries(csm_terminal PRIVATE csm_cmd)
target_compile_options(csm_terminal PRIVATE -Wall -Wextra -Wpedantic)

if(WIN32)
  set_target_properties(csm_terminal PROPERTIES
          WIN32_EXECUTABLE OFF
          RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
  )
endif()

if(BUILD_TERMINAL_TESTS)
  FetchContent_Declare(
          googletest
          GIT_REPOSITORY https://github.com/google/googletest.git
          GIT_TAG        v1.14.0
  )
  set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
  set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
  FetchContent_MakeAvailable(googletest)

  enable_testing()

  add_executable(csm_cmd_tests
          src/Tests/test_parser.cpp
          src/Tests/test_registry.cpp
          src/Tests/test_completion.cpp
          src/Tests/test_timeout.cpp
  )

  target_link_libraries(csm_cmd_tests PRIVATE csm_cmd gtest gtest_main)
  target_compile_options(csm_cmd_tests PRIVATE -Wall -Wextra -Wpedantic)

  include(GoogleTest)
  gtest_discover_tests(csm_cmd_tests)
endif()

install(TARGETS csm_cmd csm_terminal
        ARCHIVE DESTINATION lib
        LIBRARY DESTINATION lib
        RUNTIME DESTINATION bin
)
install(DIRECTORY include/ DESTINATION include)