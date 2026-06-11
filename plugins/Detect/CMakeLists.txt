juce_add_plugin(EX2Detect
    PRODUCT_NAME "EX2 Detect"
    COMPANY_NAME "EX2SEE"
    BUNDLE_ID com.ex2see.ex2detect
    PLUGIN_MANUFACTURER_CODE Ex2s
    PLUGIN_CODE Exdt
    FORMATS VST3 Standalone
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    VST3_CATEGORIES Fx Analyzer
    COPY_PLUGIN_AFTER_BUILD FALSE)

target_sources(EX2Detect PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp)

target_include_directories(EX2Detect PRIVATE Source)

target_compile_definitions(EX2Detect PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0)

target_link_libraries(EX2Detect
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags)
