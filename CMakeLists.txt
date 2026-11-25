cmake_minimum_required(VERSION 3.22)

# Static MSVC runtime: the .vst3 will not need the VC++ redistributable.
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

project(GAZYAVA VERSION 1.0.0 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        8.0.4
    GIT_SHALLOW    ON)
FetchContent_MakeAvailable(JUCE)

juce_add_plugin(GAZYAVA
    COMPANY_NAME            "GAZYAVA"
    PLUGIN_MANUFACTURER_CODE Gzva
    PLUGIN_CODE             Gz01
    FORMATS                 VST3
    PRODUCT_NAME            "GAZYAVA"
    VST3_CATEGORIES         Fx
    IS_SYNTH                FALSE
    NEEDS_MIDI_INPUT        FALSE
    NEEDS_MIDI_OUTPUT       FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE)

target_sources(GAZYAVA PRIVATE
    Source/Parameters.cpp
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
    Source/DSP/StutterProcessor.cpp
    Source/DSP/ReverseProcessor.cpp)

target_include_directories(GAZYAVA PRIVATE Source)

target_compile_definitions(GAZYAVA PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0)

target_link_libraries(GAZYAVA
    PRIVATE juce::juce_audio_utils
    PUBLIC  juce::juce_recommended_config_flags
            juce::juce_recommended_lto_flags
            juce::juce_recommended_warning_flags)
