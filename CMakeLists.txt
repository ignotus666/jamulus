cmake_minimum_required(VERSION 3.15)

project(JamulusVST VERSION 1.0.0 LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON) # Enable MOC for Q_OBJECT

# Definitions
add_compile_definitions(VST_PLUGIN)
add_compile_definitions(APP_VERSION="1.0.0")
add_compile_definitions(CUSTOM_MODES) # For Opus
add_compile_definitions(HEADLESS)

# JUCE
include(FetchContent)
FetchContent_Declare(
    juce
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        master
)
FetchContent_MakeAvailable(juce)

# Qt
find_package(Qt5 COMPONENTS Core Network Concurrent Xml REQUIRED)

# Opus
set(OPUS_CUSTOM_MODES ON CACHE BOOL "" FORCE)
add_subdirectory(libs/opus)

# Jamulus Sources
file(GLOB_RECURSE JAMULUS_SOURCES
    "src/*.cpp"
    "src/*.h"
    "src/sound/vst/*.cpp"
    "src/sound/vst/*.h"
    "src/sound/soundbase.cpp"
    "src/sound/soundbase.h"
)

# Filter Sources
list(FILTER JAMULUS_SOURCES EXCLUDE REGEX "src/main.cpp")
list(FILTER JAMULUS_SOURCES EXCLUDE REGEX "src/sound/asio/.*")
list(FILTER JAMULUS_SOURCES EXCLUDE REGEX "src/sound/jack/.*")
list(FILTER JAMULUS_SOURCES EXCLUDE REGEX "src/sound/coreaudio-mac/.*")
list(FILTER JAMULUS_SOURCES EXCLUDE REGEX "src/sound/coreaudio-ios/.*")
list(FILTER JAMULUS_SOURCES EXCLUDE REGEX "src/sound/oboe/.*")

# Exclude GUI files
list(FILTER JAMULUS_SOURCES EXCLUDE REGEX "src/.*dlg\\.cpp")
list(FILTER JAMULUS_SOURCES EXCLUDE REGEX "src/.*dlg\\.h")
list(FILTER JAMULUS_SOURCES EXCLUDE REGEX "src/analyzerconsole.*")
list(FILTER JAMULUS_SOURCES EXCLUDE REGEX "src/audiomixerboard.*")
list(FILTER JAMULUS_SOURCES EXCLUDE REGEX "src/levelmeter.*") # Exclude all levelmeter files (h/cpp/orig)
list(FILTER JAMULUS_SOURCES EXCLUDE REGEX "src/multicolorled.*")

# Plugin Sources
set(PLUGIN_SOURCES
    src/PluginProcessor.cpp
    src/PluginProcessor.h
    src/PluginEditor.cpp
    src/PluginEditor.h
)

# Create the plugin
juce_add_plugin(JamulusVST
    COMPANY_NAME "Jamulus"
    PLUGIN_MANUFACTURER_CODE "Jaml"
    PLUGIN_CODE "Jaml"
    FORMATS VST3
    PRODUCT_NAME "JamulusVST"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS TRUE
)

target_sources(JamulusVST PRIVATE
    ${JAMULUS_SOURCES}
    ${PLUGIN_SOURCES}
)

# Include Directories
target_include_directories(JamulusVST PRIVATE
    src
    src/sound/vst
    # Removed stubs path as we don't use it anymore
    libs/opus/include
    libs/opus/celt
    libs/opus/src
)

# Link Libraries
target_link_libraries(JamulusVST PRIVATE
    Qt5::Core
    Qt5::Network
    Qt5::Concurrent
    Qt5::Xml
    opus
    juce::juce_audio_utils
    juce::juce_gui_basics
    juce::juce_audio_processors
)
