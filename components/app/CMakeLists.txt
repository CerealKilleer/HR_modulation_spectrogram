idf_component_register(
    SRCS "src/app.c"
    INCLUDE_DIRS "include/"
    REQUIRES adc_continuous_mode modulation_spectrogram
)