cmake_minimum_required(VERSION 3.14)
project(my_project)

include_directories("${CMAKE_CURRENT_SOURCE_DIR}/include") 
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/src") 

# GoogleTest requires at least C++11
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS_DEBUG "-g -DDEBUG")
include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/609281088cfefc76f9d0ce82e1ff6c30cc3591e5.zip
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

enable_testing()

add_executable(
  fabrec_test
  tests/source_sink.cpp 
  tests/transform.cpp 
  tests/distribute.cpp 
)

target_link_libraries(
  fabrec_test
  gtest_main
)

include(GoogleTest)
gtest_discover_tests(fabrec_test)
