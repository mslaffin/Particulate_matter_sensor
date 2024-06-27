#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_timer.h"
#include "esp_intr_alloc.h"

#define PWM_PIN GPIO_NUM_4

volatile int64_t pulseStartTime = 0;
volatile int64_t lowPulseDuration = 0;
bool pulseStarted = false;

static void IRAM_ATTR gpio_isr_handler(void* arg) {
    int currentLevel = gpio_get_level(PWM_PIN);
    if (currentLevel == 0) {
        // Rising edge: the pulse has ended
        if (pulseStarted) {
            lowPulseDuration = esp_timer_get_time() - pulseStartTime;
            pulseStarted = false;
        }
    } else {
        // Falling edge: the pulse starts
        pulseStartTime = esp_timer_get_time();
        pulseStarted = true;
    }
}

static void configure_gpio() {
    gpio_config_t io_conf;
    io_conf.intr_type = GPIO_INTR_ANYEDGE;
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pin_bit_mask = (1ULL << PWM_PIN);
    io_conf.pull_down_en = 0;
    io_conf.pull_up_en = 1;
    gpio_config(&io_conf);

    gpio_install_isr_service(ESP_INTR_FLAG_LEVEL1);
    gpio_isr_handler_add(PWM_PIN, gpio_isr_handler, NULL);
}

void dust_sensor_task(void* pvParameters) {
    while (1) {
        if (lowPulseDuration > 0) {
            int64_t dustConcentration = lowPulseDuration / 1000; // Convert microseconds to milliseconds
            printf("Dust Concentration: %lld ug/m3\n", dustConcentration);

            lowPulseDuration = 0; // Reset duration for the next cycle
        }
        vTaskDelay(pdMS_TO_TICKS(1200));
    }
}

void app_main(void) {
    configure_gpio();
    xTaskCreate(dust_sensor_task, "dust_sensor_task", 2048, NULL, 10, NULL);
}