#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/projdefs.h"
#include "freertos/task.h"
#include "app.h"

void app_main(void)
{
    ad8232_t params = {
        .gpio = 4,
    };

    ad8232_init(&params);
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
