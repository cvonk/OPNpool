/**
 * RS-485 read/write support (similar to Arduino's HardwareSerial class)
 */

#include <stdint.h>
#include <freertos/FreeRTOS.h>
#include <driver/gpio.h>
#include "rs485.h"

static size_t     _rxBufSize = 127;
static TickType_t _rxTimeout = (100 / portTICK_RATE_MS);
static TickType_t _txTimeout = (100 / portTICK_RATE_MS);

void Rs485::begin(uart_config_t * uartConfig, int rxPin, int txPin, int rtsPin) 
{
/*
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << rxPin),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        io_conf.intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io_conf);

    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = ((1ULL << txPin) | (1ULL << rtsPin));
    gpio_config(&io_conf);
*/
#if 1
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << rtsPin),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        io_conf.intr_type = GPIO_INTR_DISABLE
    };
    ESP_ERROR_CHECK( gpio_config(&io_conf) );
    gpio_set_level((gpio_num_t)rtsPin, 0); 

    uart_param_config(_uartNr, uartConfig);
    uart_set_pin(_uartNr, txPin, rxPin, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    uart_driver_install(_uartNr, _rxBufSize * 2, 0, 0, NULL, 0);  // no tx buffer
    uart_set_mode(_uartNr, UART_MODE_RS485_HALF_DUPLEX);
#else
    uart_param_config(_uartNr, uartConfig);
    uart_set_pin(_uartNr, txPin, rxPin, rtsPin, UART_PIN_NO_CHANGE);
    uart_driver_install(_uartNr, _rxBufSize * 2, 0, 0, NULL, 0);  // no tx buffer
    uart_set_mode(_uartNr, UART_MODE_RS485_HALF_DUPLEX);
#endif
}

void Rs485::end() 
{
    flush();
}

int Rs485::available(void) 
{
    int length = 0;
    ESP_ERROR_CHECK(uart_get_buffered_data_len(_uartNr, (size_t *) &length));
    return length;
}

uint8_t Rs485::read(void) 
{
    uint8_t dst;
    readBytes(&dst, 1);
    return dst;
}

int Rs485::readBytes(uint8_t * dst, uint32_t len) 
{
    return uart_read_bytes(_uartNr, dst, len, _rxTimeout);
}

void Rs485::flush(void) 
{
    ESP_ERROR_CHECK(uart_wait_tx_done(_uartNr, _txTimeout)); // wait timeout is 100 RTOS ticks (TickType_t)
    ESP_ERROR_CHECK(uart_flush_input(_uartNr));
}

size_t Rs485::write(uint8_t src) 
{
    return uart_write_bytes(_uartNr, (char *) &src, 1);
}

size_t Rs485::writeBytes(uint8_t * src, size_t len) 
{
    return uart_write_bytes(_uartNr, (char *) src, len);
}

#if 0
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "driver/uart.h"
#include "freertos/queue.h"
#include "esp_log.h"
#include "soc/uart_struct.h"

define RX_BUF_SIZE             (127)
#define RS485_READ_TIMEOUT      (100 / portTICK_RATE_MS)
#define RS485_TASK_STACK_SIZE   (2048)
#define RS485_TASK_PRIO         (10)
#define RS485_UART_PORT         (UART_NUM_2)

// UART2 default pins IO16, IO17 do not work on ESP32-WROVER module because these pins connected to PSRAM
#define TXD_PIN (GPIO_NUM_23)
#define RXD_PIN (GPIO_NUM_22)
#define RTS_PIN (GPIO_NUM_18)  /* DE/~DE */

void init()
{
    uart_port_t const uart_num = RS485_UART_PORT;
    uart_config_t uart_config = {
        .baud_rate = 9600,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .rx_flow_ctrl_thresh = 122
    };
    uart_param_config(uart_num, &uart_config);
    uart_set_pin(uart_num, TXD_PIN, RXD_PIN, RTS_PIN, UART_PIN_NO_CHANGE);
    uart_driver_install(uart_num, RX_BUF_SIZE * 2, 0, 0, NULL, 0);  // no tx buffer
    uart_set_mode(uart_num, UART_MODE_RS485_HALF_DUPLEX);
}

static void RS485_RxTask(void * params)
{
    static char const * const TAG = "PoolIf/RS485/Rx";
    esp_log_level_set(TAG, ESP_LOG_INFO);

    uint8_t * const data = (uint8_t *) malloc(RX_BUF_SIZE);
    while (1) {
        const int rxBytes = uart_read_bytes(RS485_UART_PORT, data, RX_BUF_SIZE, RS485_READ_TIMEOUT);
        if (rxBytes > 0) {
            ESP_LOGI(TAG, "Read %d bytes: '%s'", rxBytes, data);
            ESP_LOG_BUFFER_HEXDUMP(TAG, data, rxBytes, ESP_LOG_INFO);
        }
    }
    free(data);
}

static void RS485_TxTask(void * params)
{
    static const char * TAG = "PoolIf/RS485/Tx";
    esp_log_level_set(TAG, ESP_LOG_INFO);
    while (1) {
        //sendData(TAG, "Hello world");
        vTaskDelay(2000 / portTICK_PERIOD_MS);
    }
}

void pool_main()
{
    init();
    xTaskCreate(RS485_RxTask, "rs485_rx_task", 1024*2, NULL, configMAX_PRIORITIES, NULL);
    xTaskCreate(RS485_TxTask, "rs485_tx_task", 1024*2, NULL, configMAX_PRIORITIES-1, NULL);
}
#endif