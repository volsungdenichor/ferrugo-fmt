cmake_minimum_required(VERSION 3.5)
project(ferrugo-fmt)

enable_testing()
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17")

add_subdirectory(tests)

include(FetchContent)

FetchContent_Declare(ferrugo-core
    GIT_REPOSITORY git@github.com:volsungdenichor/ferrugo-core.git
    GIT_TAG main)

FetchContent_MakeAvailable(ferrugo-core)

include_directories(${ferrugo-core_SOURCE_DIR}/include)
