/**
 * RS-485 read/write support (similar to Arduino's HardwareSerial class)
 */

#pragma once
#include <inttypes.h>
#include <driver/uart.h>

class Rs485 {

  public:
    Rs485(uart_port_t uartNr) : _uartNr(uartNr) {};
    void begin(uart_config_t * uart_config, int rxPin, int txPin, int rtsPin);
    void end();
    int available(void);
    uint8_t read(void);
    int readBytes(uint8_t * dst, uint32_t len);
    void flush(void);
    size_t write(uint8_t src);
    size_t writeBytes(uint8_t * src, size_t len);

  protected:
    uart_port_t _uartNr;
};