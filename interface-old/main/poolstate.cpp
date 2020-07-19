/**
 * State based in Various support function for transmitting/receiving over the Pentair RS-485 bus
 * 
 * 2BD: should serialize access (for env with multiple threads/tasks)
 */

#include <ArduinoJson.h>  // https://github.com/bblanchon/ArduinoJson
// required for ArduinoJson, see http://www.avrfreaks.net/comment/341297#comment-341297
__extension__ typedef int __guard __attribute__((mode(__DI__)));
#if 0
extern "C" int __cxa_guard_acquire(__guard *);
extern "C" void __cxa_guard_release(__guard *);
extern "C" void __cxa_guard_abort(__guard *);
int __cxa_guard_acquire(__guard *g) { return !*(char *)(g); };
void __cxa_guard_release(__guard *g) { *(char *)g = 1; };
void __cxa_guard_abort(__guard *) { };
#endif

#include <stdint.h>
#include <sdkconfig.h>
#include "rs485.h"
#include "pooltypes.h"  // seperate file to workaround Arduino IDE reordering typedef and functions
#include "encode.h"
#include "utils.h"
#include "receiver.h"
#include "transmitter.h"
#include "poolstate.h"

static char const * const TAG = "PoolIf/poolstate";

#define ARRAY_SIZE(a) (sizeof(a) / sizeof(a[0]))

void
PoolState::update(sysState_t * sys) {
	memcpy(this->state_, sys, sizeof(*this->state_));
}

size_t
PoolState::getStateAsJson(char * buffer, size_t bufferLen)
{
	StaticJsonBuffer<CONFIG_POOL_JSON_BUFSIZE> jsonBuffer;
	JsonObject * json = &jsonBuffer.createObject();

	sysState_t * sys = this->state_;
	Utils::resetIdx();

	if (json && sys) {
		{
			JsonObject & obj = json->createNestedObject("tod");
			obj["time"] = Utils::strTime(sys->time.hour, sys->time.minute);
			obj["date"] = Utils::strDate(sys->date.year, sys->date.month, sys->date.day);
		} {
			JsonObject & obj = json->createNestedObject("pool");
			obj["temp"] = sys->pool.temp;
			obj["sp"] = sys->pool.setPoint;
			obj["src"] = Utils::strHeatSrc(sys->pool.heatSrc);
			obj["heating"] = sys->pool.heating;
		} {
			JsonObject & obj = json->createNestedObject("spa");
			obj["temp"] = sys->spa.temp;
			obj["sp"] = sys->spa.setPoint;
			obj["src"] = Utils::strHeatSrc(sys->spa.heatSrc);
			obj["heating"] = sys->spa.heating;
		} {
			JsonObject & obj = json->createNestedObject("air");
			obj["temp"] = sys->air.temp;
		} {
			Utils::activeCircuits(*json, "active", sys->circuits.active);
#if 0
			jsonActive(*json, "delay", sys->circuits.delay);
#endif
		} {
			JsonObject & obj = json->createNestedObject("chlor");
			obj["salt"] = sys->chlor.salt;
			obj["pct"] = sys->chlor.pct;
			obj["status"] = Utils::chlorStateName(sys->chlor.state);
		} {
			JsonObject & obj = json->createNestedObject("schedule");
			for (uint_least8_t ii = 0; ii < ARRAY_SIZE(sys->sched); ii++) {
				poolStateSched_t * sched = &sys->sched[ii];
				if (sched->circuit) {
					Utils::schedule(obj, sched->circuit, sched->start, sched->stop);
				}
			}
		} {
			JsonObject & obj = json->createNestedObject("pump");
			obj["running"] = sys->pump.running;
			obj["mode"] = Utils::strPumpMode(sys->pump.mode);
			obj["status"] = sys->pump.status;
			obj["pwr"] = sys->pump.pwr;
			obj["rpm"] = sys->pump.rpm;
			obj["err"] = sys->pump.err;
		}
		return Utils::jsonPrint(json, buffer, bufferLen);
	}
	return 0;
}

/*
* main entry points
*/

void
PoolState::begin()
{
	this->state_ = (sysState_t *)malloc(sizeof(sysState_t));
	if (!this->state_) {
		printf("xNOMEM\n");
	}
	memset(this->state_, 0, sizeof(*this->state_));
}

#if 0
bool  // return true when there is a transmit opportunity (a complete message has been received)
PoolState::receive(HardwareSerial * rs485) {

	return Receiver::receive(rs485, this->state_);
}
#endif

heatState_t
PoolState::getHeatState(void)
{
	heatState_t current;
	current.pool.setPoint = this->state_->pool.setPoint;
	current.pool.heatSrc = this->state_->pool.heatSrc;
	current.spa.setPoint = this->state_->spa.setPoint;
	current.spa.heatSrc = this->state_->spa.heatSrc;
	return current;
}

bool
PoolState::getCircuitState(char const * const key)
{
	return Utils::circuitIsActive(key, this->state_->circuits.active);
}

uint8_t
PoolState::getSetPoint(char const * const key)
{
	if (strcmp(key, "pool") == 0) {		
		return this->state_->pool.setPoint;
	} 
	if (strcmp(key, "spa") == 0) {		
		return this->state_->spa.setPoint;
	} 
	return 0;
}

char const *
PoolState::getHeatSrc(char const * const key)
{
	if (strcmp(key, "pool") == 0) {		
		return Utils::strHeatSrc(this->state_->pool.heatSrc);
	}
	if (strcmp(key, "spa") == 0) {		
		return Utils::strHeatSrc(this->state_->spa.heatSrc);
	}
	return "err";
}
