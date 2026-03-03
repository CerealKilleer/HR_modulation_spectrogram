idf_component_register(
    SRCS "src/tcp_client.c"
    INCLUDE_DIRS "include/"
    PRIV_REQUIRES nvs_flash esp_netif
)