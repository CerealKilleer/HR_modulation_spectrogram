idf_component_register(SRCS "main.c"
                    INCLUDE_DIRS "."
                    REQUIRES ad8232 modulation_spectrogram wifi tcp_client)
