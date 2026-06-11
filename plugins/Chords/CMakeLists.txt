juce_add_plugin(EX2Chords
    PRODUCT_NAME "EX2 Chords"
    COMPANY_NAME "EX2SEE"
    BUNDLE_ID com.ex2see.ex2chords
    PLUGIN_MANUFACTURER_CODE Ex2s
    PLUGIN_CODE Exch
    FORMATS VST3 Standalone
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT TRUE
    IS_MIDI_EFFECT FALSE
    VST3_CATEGORIES Instrument
    COPY_PLUGIN_AFTER_BUILD FALSE)

target_sources(EX2Chords PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp)

target_include_directories(EX2Chords PRIVATE Source)

target_compile_definitions(EX2Chords PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0)

target_link_libraries(EX2Chords
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags)
