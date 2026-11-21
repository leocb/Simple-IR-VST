cmake_minimum_required(VERSION 3.22)

project(IRLoader VERSION 1.0.1 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (APPLE)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "Minimum macOS deployment target")
endif()

option(IRLOADER_COPY_PLUGINS_AFTER_BUILD
       "Copy built plugins into the current user's plugin folders" ON)

# JUCE is vendored in third_party/JUCE so builds work offline. If the folder is
# missing it is downloaded automatically on the first configure.
set(JUCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/third_party/JUCE")

if (EXISTS "${JUCE_DIR}/CMakeLists.txt")
    add_subdirectory("${JUCE_DIR}")
else()
    message(STATUS "JUCE not found locally - downloading JUCE 9.0.6")
    include(FetchContent)
    FetchContent_Declare(JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG 9.0.6
        GIT_SHALLOW TRUE
        SOURCE_DIR "${JUCE_DIR}")
    FetchContent_MakeAvailable(JUCE)
endif()

# AU is an Apple-only format; Windows builds VST3 and the standalone app.
set(IRLOADER_FORMATS VST3 Standalone)

if (APPLE)
    list(APPEND IRLOADER_FORMATS AU)
endif()

juce_add_plugin(IRLoader
    COMPANY_NAME "Leocb"
    BUNDLE_ID "com.leocb.irloader"
    PLUGIN_MANUFACTURER_CODE Leoc
    PLUGIN_CODE IrLd
    PRODUCT_NAME "IR Loader"
    DESCRIPTION "Impulse response loader with a wet/dry mix"
    FORMATS ${IRLOADER_FORMATS}
    AU_MAIN_TYPE kAudioUnitType_Effect
    VST3_CATEGORIES Fx
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD ${IRLOADER_COPY_PLUGINS_AFTER_BUILD})

target_sources(IRLoader PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp)

target_compile_definitions(IRLoader PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_REPORT_APP_USAGE=0
    JUCE_STRICT_REFCOUNTEDPOINTER=1)

target_link_libraries(IRLoader
    PRIVATE
    juce::juce_audio_utils
    juce::juce_dsp
    PUBLIC
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags)

# Offline tests that exercise the audio processor directly (no host required).
option(IRLOADER_BUILD_TESTS "Build the IR Loader processor tests" ON)

if (IRLOADER_BUILD_TESTS)
    add_executable(IRLoaderTests
        Tests/ProcessorTests.cpp
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp)

    target_include_directories(IRLoaderTests PRIVATE Source)

    target_compile_definitions(IRLoaderTests PRIVATE
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_STANDALONE_APPLICATION=1
            JUCE_REPORT_APP_USAGE=0
        JucePlugin_Name="IR Loader")

    target_link_libraries(IRLoaderTests
        PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags)
endif()
