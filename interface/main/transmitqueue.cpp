/**
 * Queue for messages to be transmitted over the Pentair RS-485 bus
 */

#include <stdio.h>
#include <inttypes.h>
#include "cbuffer.h"
#include <sdkconfig.h>
#include <esp_log.h>

#include "ArduinoJson.h"
#include "rs485.h"
#include "pooltypes.h"
#include "poolstate.h"
#include "utils.h"
#include "encode.h"
#include "transmitter.h"
#include "transmitqueue.h"

static char const * const TAG = "pool_transmitqueue";

void
TransmitQueue::begin(void)
{
	static MT_CTRL_a5_t mt_reqs[] = { MT_CTRL_heatReq, MT_CTRL_schedReq, MT_CTRL_timeReq };

	for (uint_least8_t ii = 0; ii < ARRAY_SIZE(mt_reqs); ii++) {
		element_t element = {
			.typ = mt_reqs[ii],
			.dataLen = 0,
			.data = {}
		};
		ESP_LOGI(TAG, "queuing init %02X", element.typ);
		this->txBuffer_.push(element);
		//send_a5( rs485, dirPin, root, sys, mt_reqs[idx], &hdr, NULL, 0 );
	}
}

void
TransmitQueue::transmit(Rs485 * rs485)
{
	StaticJsonBuffer<CONFIG_POOL_JSON_BUFSIZE> jsonBuffer;
	JsonObject * root = &jsonBuffer.createObject();

	if (!this->txBuffer_.isEmpty()) {
		element_t * element = this->txBuffer_.popPtr();
		ESP_LOGI(TAG, "requesting %02X", element->typ);
		Transmitter::send_a5(rs485, root, element);
		//printf("%ul\n", ESP.getFreeHeap());
	}
}

void
TransmitQueue::enqueue(char const * const name, char const * const valuestr)
{
	StaticJsonBuffer<CONFIG_POOL_JSON_BUFSIZE> jsonBuffer;
	JsonObject * json = &jsonBuffer.createObject();

	if (json) {
		//HardwareSerial * rs485 = this->rs485_;
		//uint_least8_t dirPin = this->rs485DirPin_;

		char label[5];
		char const * subLbl = NULL;
		for (uint_least8_t ii = 0; ii < strlen(name) && !subLbl; ii++) {
			if (name[ii] == '-') {
				subLbl = &name[ii + 1];
			}
			else {
				if (ii < sizeof(label) - 1) {
					label[ii] = name[ii];
					label[ii + 1] = '\0';
				}
			}
		}
		uint_least8_t const circuit = Utils::circuitNr(label);

		// the library function strtok() doesn't appear to work correctly on ESP8266
#if 0
		char * labelStr = strtok(label, "-"); // e.g. spa-sp=100, pool-active=1, spa-src=heater
		char * subLblStr = strtok(NULL, "=");
#endif

		if (circuit && *subLbl && *valuestr) {

			uint8_t const newValue = atoi(valuestr);
			//element_t element;
			if (strcmp(subLbl, "active") == 0) {
				ESP_LOGI(TAG, "queuing circuitmsg set circuit %d to %d", circuit, newValue);
				element_t element;
				
				EncodeA5::circuitMsg(&element, circuit, newValue);
				this->txBuffer_.push(element);
			} 
			else if (circuit == CIRCUITNR_spa || circuit == CIRCUITNR_pool) {
				ESP_LOGI(TAG, "queuing heatmsg\n");

				bool const setHeatSp = strcmp(subLbl, "sp") == 0;
				bool const setHeatSrc = strcmp(subLbl, "src") == 0;
				element_t element;
                heatState_t const oldValue = this->poolState_.getHeatState();

				EncodeA5::heatMsg(&element, 
					setHeatSp && circuit == CIRCUITNR_pool ? newValue : oldValue.pool.setPoint,
					setHeatSp && circuit == CIRCUITNR_spa ? newValue : oldValue.spa.setPoint,
					(setHeatSrc && circuit == CIRCUITNR_pool ? newValue : oldValue.pool.heatSrc) |
					(setHeatSrc && circuit == CIRCUITNR_spa ? newValue : oldValue.spa.heatSrc) << 2);
				this->txBuffer_.push(element);
			}
		}
	}
}


