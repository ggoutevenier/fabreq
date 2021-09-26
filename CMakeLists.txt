cmake_minimum_required(VERSION 3.14)
project(fabreq)

set(ENV{LD_LIBRARY_PATH} /usr/local/lib)

include_directories("${CMAKE_CURRENT_SOURCE_DIR}/include") 
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/src") 

# GoogleTest requires at least C++11
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS_DEBUG "-g -DDEBUG")

#set(CMAKE_CXX_FLAGS "-fsanitize=address  -fsanitize=leak -g")
#CMAKE_EXE_LINKER_FLAGS:STRING=-fsanitize=address  -fsanitize=leak
#CMAKE_MODULE_LINKER_FLAGS:STRING=-fsanitize=address  -fsanitize=leak

include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/609281088cfefc76f9d0ce82e1ff6c30cc3591e5.zip
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

enable_testing()

#add_executable(
#  fabrec_test
#  tests/source_sink.cpp 
#  tests/transform.cpp 
#  tests/distribute.cpp 
#  tests/clone.cpp
#  tests/gtest.cpp
#)
add_executable(
  presentation
  presentation/main.cpp
)

target_link_libraries(
  presentation
  pthread
#  fabrec_test
#  gtest_main
#  gtest
#  log4cxx
)

#include(GoogleTest)
#gtest_discover_tests(fabrec_test)

