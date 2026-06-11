cmake_minimum_required(VERSION 3.22)
project(EX2Plugins VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

include(FetchContent)
FetchContent_Declare(JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        8.0.12
    GIT_SHALLOW    TRUE)
FetchContent_MakeAvailable(JUCE)

add_subdirectory(plugins/Chords)
add_subdirectory(plugins/Detect)
