#ifndef __SPEC_MQTT_CLIENT_H__
#define __SPEC_MQTT_CLIENT_H__
#include <stdint.h>
void mqtt_client_init();
void mqtt_client_publish_hr(float hr);
void mqtt_client_publish_ecg(uint16_t *ecg_buffer, uint8_t size);
#endif