/*
 * SPDX-FileCopyrightText: 2021-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */

#include <stdio.h>
#include <string.h>
#include "esp_system.h"
#include "esp_console.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/rmt_tx.h"
#include "driver/rmt_rx.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "manchester_encoder.h"
#include <stdlib.h>
#include "esp_vfs_dev.h"
#include "esp_vfs_fat.h"
#include "driver/uart.h"

#define TX_PIN 18
#define RX_PIN 19

#define TICK_RESOLUTION 80 * 1000 * 1000 // 50MHz resolution

static const char *TAG = "Manchester Encoding Transmit";

void app_main(void) {

    srand(0);

    ESP_LOGI(TAG, "Create RMT TX channel");
    rmt_channel_handle_t tx_channel = NULL;
    rmt_tx_channel_config_t tx_chan_config = {
        .clk_src = RMT_CLK_SRC_DEFAULT, // select clock source
        .gpio_num = TX_PIN,
        .mem_block_symbols = 64,
        .resolution_hz = TICK_RESOLUTION,
        .trans_queue_depth = 3, // set the number of transactions that can be pending in the background
    };
    ESP_ERROR_CHECK(rmt_new_tx_channel(&tx_chan_config, &tx_channel));

    // rmt_carrier_config_t tx_carrier_cfg = {
    //     .duty_cycle = 0.5,                 // duty cycle 33%
    //     .frequency_hz = 10000000,              // 38 KHz
    //     .flags.polarity_active_low = false, // carrier should be modulated to high level
    // };
    // // modulate carrier to TX channel
    // ESP_ERROR_CHECK(rmt_apply_carrier(tx_channel, &tx_carrier_cfg));

    ESP_LOGI(TAG, "Create encoders");

    manchester_encoder_config_t manchester_encoder_config = {
        .resolution = TICK_RESOLUTION,
    };
    rmt_encoder_handle_t manchester_encoder = NULL;
    ESP_ERROR_CHECK(rmt_new_manchester_encoder(&manchester_encoder_config, &manchester_encoder));

    rmt_transmit_config_t tx_config = {
        .loop_count = 0,
    };

    ESP_LOGI(TAG, "Enable RMT channel");
    ESP_ERROR_CHECK(rmt_enable(tx_channel));

    uint32_t i = 0xFFFFFFF0;

    setvbuf(stdin, NULL, _IONBF, 0);
    setvbuf(stdout, NULL, _IONBF, 0);
    ESP_ERROR_CHECK(uart_driver_install(CONFIG_ESP_CONSOLE_UART_NUM, 256, 0, 0, NULL, 0));
    esp_vfs_dev_uart_use_driver(CONFIG_ESP_CONSOLE_UART_NUM);
    esp_vfs_dev_uart_port_set_rx_line_endings(CONFIG_ESP_CONSOLE_UART_NUM, ESP_LINE_ENDINGS_CR);
    esp_vfs_dev_uart_port_set_tx_line_endings(CONFIG_ESP_CONSOLE_UART_NUM, ESP_LINE_ENDINGS_CRLF);

    while(1) {
        scanf("%ld", &i);
        ESP_ERROR_CHECK(rmt_transmit(tx_channel, manchester_encoder, &i, sizeof(uint32_t), &tx_config));
        ESP_LOGI(TAG, "Sent Frame: 0x%lx", i);
        // i++;
        // vTaskDelay(20/portTICK_PERIOD_MS);
    }


}
