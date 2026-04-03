#include "tcp_client.h"
#include "freertos/FreeRTOS.h"
#include "freertos/idf_additions.h"
#include "freertos/projdefs.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include "esp_netif.h"
#include "esp_log.h"
#include "lwip/sockets.h"
#include "portmacro.h"
#include <errno.h>

#define HOST_IP "192.168.88.196"
#define HOST_PORT 1234
static const char *TAG = "tcp_client_task";

extern float *mod_spectrogram;

static int tcp_client_init(void)
{
    struct sockaddr_in dest_addr;
    inet_pton(AF_INET, HOST_IP, &dest_addr.sin_addr);
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(HOST_PORT);

    int sock = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);

    if (sock < 0) {
        ESP_LOGE(TAG, "No se pudo crear el socket");
        return -1;
    }

    ESP_LOGI(TAG, "Socket creado, conectando a: %s:%d", HOST_IP, HOST_PORT);

    int err = connect(sock, (struct sockaddr *)&dest_addr, sizeof(dest_addr));

    if (err != 0) {
        ESP_LOGE(TAG, "No se pudo conectar al destino. Errno %d", errno);
        return -1;
    }

    ESP_LOGI(TAG, "Conexión exitosa a: %s:%d", HOST_IP, HOST_PORT);
    return sock;
}

void tcp_client(void *args)
{
    int sock = tcp_client_init();
    
    if (sock < 0) {
        vTaskDelete(NULL);
    }

    while (1) {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        uint8_t *ptr = (uint8_t *)mod_spectrogram;
        int bytes_to_send = 257 * 257 * sizeof(float);

        while (bytes_to_send > 0) {
            int sent = send(sock, ptr, bytes_to_send, 0);

            if (sent < 0) {
                ESP_LOGE(TAG, "Error enviando. Errno %d", errno);
                break;
            }

            ptr += sent;
            bytes_to_send -= sent;
        }
    }

    if (sock != -1) {
        shutdown(sock, 0);
        close(sock);
    }
    
    vTaskDelete(NULL);
}