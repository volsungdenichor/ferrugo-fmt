include(FetchContent)

FetchContent_Declare(ferrugo-core
    GIT_REPOSITORY https://github.com/volsungdenichor/ferrugo-core.git
    GIT_TAG main)

FetchContent_MakeAvailable(ferrugo-core)

include_directories(${ferrugo-core_SOURCE_DIR}/include)
