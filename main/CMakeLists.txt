idf_component_register(SRCS "main.c"
                    INCLUDE_DIRS "."
                    REQUIRES ad8232 modulation_spectrogram wifi spec_mqtt_client tcp_client)
