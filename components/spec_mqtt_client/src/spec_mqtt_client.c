#include "mqtt_client.h"
#include "esp_log.h"
#include "spec_mqtt_client.h"
#include <stdio.h>
#include <string.h>

#define MQTT_TOPIC "/hr"
esp_mqtt_client_handle_t client;
static const char *TAG = "MQTT_CLIENT";

extern const uint8_t ca_crt_start[] asm("_binary_ca_crt_start");
extern const uint8_t ca_crt_end[]   asm("_binary_ca_crt_end");

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    const esp_mqtt_event_handle_t event = event_data;
    const esp_mqtt_client_handle_t client = event->client;

    switch (event_id) {
    case MQTT_EVENT_CONNECTED:
        esp_mqtt_client_subscribe(client, MQTT_TOPIC, 1);
        break;
    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGI("TAG", "Cliente desconectado");
        break;
    case MQTT_EVENT_PUBLISHED:
        ESP_LOGI(TAG, "Mensaje publicado");
        break;
    default:
        break;
    }
}


void mqtt_client_publish_hr(float hr)
{
    char msg[10];
    int len = snprintf(msg, 10, "%.2f", hr);
    esp_mqtt_client_enqueue(client, MQTT_TOPIC, msg, len, 1, 0, true);
}

void mqtt_client_init()
{
    const esp_mqtt_client_config_t mqtt_cfg = {
        .broker = {
            .address.uri = "mqtts://yonathan:Celeste246-@192.168.88.196:8883",
            .verification.certificate = (const char *)ca_crt_start,
        }
    };

    client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    esp_mqtt_client_start(client);
}