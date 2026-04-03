idf_component_register(
    SRCS "src/spec_mqtt_client.c"
    INCLUDE_DIRS "include"
    REQUIRES mqtt 
)

target_add_binary_data(${COMPONENT_LIB} "certs/ca.crt" TEXT)