/**
 * Transmit messages over the Pentair RS-485 bus
 */

#include <sdkconfig.h>
#include <stdint.h>
#include <esp_log.h>
#include <esp32/rom/ets_sys.h>
#include <driver/gpio.h>
#include "rs485.h"
#include "utils.h"
#include "decode.h"
#include "utils.h"
#include "transmitqueue.h"
#include "transmitter.h"

static char const * const TAG = "pool_transmitter";

static uint8_t datalink_preamble_a5[] = { 0x00, 0xFF, 0xA5 };
//static uint8_t datalink_preamble_ic[] = { 0x10, 0x02 };

void
Transmitter::send_a5(Rs485 * rs485, JsonObject * root, element_t const * const element)
{
	ESP_LOGI(TAG, "sending %02X", element->typ);
	mHdr_a5_t const hdr = {
		.pro = 0x01,
		.dst = Utils::addr(ADDRGROUP_ctrl, 0),
		.src = Utils::addr(ADDRGROUP_remote, 0),
		.typ = element->typ,
		.len = element->dataLen
	};
	uint16_t const crc = Utils::calcCRC_a5(&hdr, element->data.raw);

	// messages should be sent directly after an A5 packets (and before any IC packets)

	// enable RS485 transmit DE=1 and RE*=1 (DE=driver enable, RE*=inverted receive enable)
	//CJV?? digitalWrite(dirPin, RS485_DIR_tx);  // 2BD: there might be a mandatory wait after enabling this pin !!!!!!!
    gpio_set_level(GPIO_NUM_27, 1);
	{
		rs485->write(0xFF);
		for (uint_least8_t ii = 0; ii < sizeof(datalink_preamble_a5); ii++) {
			rs485->write(datalink_preamble_a5[ii]);
		}
		for (uint_least8_t ii = 0; ii < sizeof(mHdr_a5_t); ii++) {
			rs485->write(((uint8_t *)&hdr)[ii]);
		}

		printf("DBG typ=%02X dataLen=%u data.circuitSet=(%02X %02X)", element->typ, element->dataLen, element->data.circuitSet.circuit, element->data.circuitSet.value);
		for (uint_least8_t ii = 0; ii < hdr.len; ii++) {
			printf(" %02X", element->data.raw[ii]);
			rs485->write(element->data.raw[ii]);
		}
		printf("\n");
		rs485->write(crc >> 8);
		rs485->write(crc & 0xFF);
		//rs485->write(0xFF);
		rs485->flush();  // wait until the hardware buffer starts transmitting the last byte

		// A few words on the DE signal:
		//  - choose a GPIO that doesn't mind being pulled down during reset
		//  - in an ideal world, the UART has a tx interrupt, or at least a tx-complete bit, so
		//    that a transmit-done callback can off the DE.
		//  - probing the TX and DE signal, I learned that the DE is 1 byte time (10/9600 sec) to
		//    short.  Correct with a delay() statement.

		//vTaskDelay(1/portTICK_PERIOD_MS);
	}
	ets_delay_us(1500);
    gpio_set_level(GPIO_NUM_27, 0);  // enable RS485 receive

#if 1
	pentairMsg_t msg = {};
	msg.proto = PROTOCOL_a5;
	msg.hdr = hdr;
	msg.chk = 0;
	memcpy(msg.data, element->data.raw, element->dataLen);
    Decode::decode(&msg, NULL, root);
	Utils::jsonRaw(&msg, root, "raw");
    Utils::jsonPrint(root, NULL, 0);
#endif
	ESP_LOGI(TAG, "sent.");
}
