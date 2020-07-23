/**
 * Decode Messages received over the Pentair RS-485 bus.
 * Status updates are returned  and returns them in JSON and as a binary state.
 */

#include <stdint.h>
#include <esp_log.h>

#include "ArduinoJson.h"
#include "pooltypes.h"
#include "poolstate.h"
#include "utils.h"
#include "decode.h"

static char const * const TAG = "pool_decode";

inline bool
decodeCtrl_setAck_a5(mCtrlSetAck_a5_t * msg, uint_least8_t const len,
                     sysState_t * sys, JsonObject * json, char const * const key)
{
	(void) sys;
	if (len == sizeof(*msg)) {
		if (json) {
			JsonObject & obj = json->createNestedObject(key);
			obj["typ"] = Utils::mtCtrlName(static_cast<DATALINK_A5_CTRL_MSGTYP_t>(msg->typ), NULL);
		}
		return true;
	}
	return false;
}

inline bool
decodeCtrl_circuitSet_a5(mCtrlCircuitSet_a5_t * msg, uint_least8_t const len,
                         sysState_t * sys, JsonObject * json, char const * const key)
{
	(void) sys;
	printf("decodeCtrl_circuitSet_a5 msg=(%02X %02X)\n", msg->circuit, msg->value);
	if (len == sizeof(*msg)) {
		if (json) {
			JsonObject & obj = json->createNestedObject(key);
			obj[Utils::circuitName(msg->circuit)] = msg->value;
		}
		return true;
	}
	return false;
}

inline bool
decodeCtrl_variousReq_a5(uint8_t * msg, uint_least8_t const len,
                         sysState_t * sys, JsonObject * json, char const * const key)
{
	(void) sys;
	if (len == 0) {
		if (json) {
			JsonObject & obj = json->createNestedObject(key);
			(void)obj;
		}
		return true;
	}
	return false;
}

inline bool
decodeCtrl_time_a5(mCtrlTime_a5_t const * const msg, uint_least8_t const len,
                   sysState_t * sys, JsonObject * json, char const * const key)
{
	if (len == sizeof(*msg)) {
		if (json) {
			JsonObject & obj = json->createNestedObject(key);
			obj["time"] = Utils::strTime(msg->hour, msg->minute);
			obj["date"] = Utils::strDate(msg->year, msg->month, msg->day);
			//obj["dst"] = msg->daylightSavings;
			//int16_t cs = msg->clkSpeed;
			//obj["clkSpeed"] = min( cs, 60 ) * 5;  // adjustment unit=5
		}
		if (sys) {
			sys->time.minute = msg->minute;
			sys->time.hour = msg->hour;
			sys->date.day = msg->day;
			sys->date.month = msg->month;
			sys->date.year = msg->year;
		}
		return true;
	}
	return false;
}

inline bool
decodeCtrl_state_a5(mCtrlState_a5_t const * const msg, uint_least8_t const len,
                    sysState_t * sys, JsonObject * json, char const * const key)
{
	if (len == sizeof(*msg)) {
		if (json) {
			JsonObject & obj = json->createNestedObject(key);
			{
				JsonObject & system = obj.createNestedObject("system");
				system["time"] = Utils::strTime(msg->hour, msg->minute);
				system["fw"] = msg->major + (float)msg->minor / 100;
			} {
				Utils::activeCircuits(obj, "active", (msg->activeHi << 8) | msg->activeLo);
				Utils::activeCircuits(obj, "delay", msg->delayLo);
			} {
				JsonObject & pool = obj.createNestedObject("pool");
				pool["temp"] = msg->poolTemp;
				pool["src"] = Utils::strHeatSrc(msg->heatSrc & 0x03);
				pool["heating"] = (bool)(msg->heating & 0x04);
			} {
				JsonObject & spa = obj.createNestedObject("spa");
				spa["temp"] = msg->spaTemp;
				spa["src"] = Utils::strHeatSrc(msg->heatSrc >> 2);
				spa["heating"] = (bool)(msg->heating & 0x08);
			} {
				JsonObject & temp = obj.createNestedObject("temp");
				temp["air"] = msg->airtemp;
				if (msg->solartemp != 0xFF) {
					temp["solar"] = msg->solartemp;
				}
			}
		}
		if (sys) {
			sys->time.minute = msg->minute;
			sys->time.hour = msg->hour;
			sys->circuits.active = (msg->activeHi << 8) | msg->activeLo;
			sys->circuits.delay = msg->delayLo;
			sys->pool.temp = msg->poolTemp;
			sys->pool.heatSrc = msg->heatSrc & 0x03;
			sys->pool.heating = msg->heating & 0x04;
			sys->spa.temp = msg->poolTemp;
			sys->spa.heatSrc = msg->heatSrc >> 2;
			sys->spa.heating = msg->heating & 0x08;
			sys->air.temp = msg->airtemp;
		}
		return true;
	}
	return false;
}

inline bool
decodeCtrl_heat_a5(mCtrlHeat_a5_t const * const msg, uint_least8_t const len,
                   sysState_t * sys, JsonObject * json, char const * const key)
{
	if (len == sizeof(*msg)) {
		if (json) {
			JsonObject & obj = json->createNestedObject(key);
			{
				JsonObject & pool = obj.createNestedObject("pool");
				pool["temp"] = msg->poolTemp;
				pool["sp"] = msg->poolTempSetpoint;
				pool["src"] = Utils::strHeatSrc(msg->heatSrc & 0x03);
			} {
				JsonObject & spa = obj.createNestedObject("spa");
				spa["temp"] = msg->spaTemp;
				spa["sp"] = msg->spaTempSetpoint;
				spa["src"] = Utils::strHeatSrc(msg->heatSrc >> 2);
			}
		}
		if (sys) {
			sys->pool.setPoint = msg->poolTempSetpoint;
			sys->spa.setPoint = msg->spaTempSetpoint;
		}
		return true;
	}
	return false;
}

inline bool
decodeCtrl_heatSet_a5(mCtrlHeatSet_a5_t const * const msg, uint_least8_t const len,
                      sysState_t * sys, JsonObject * json, char const * const key)
{
	if (len == sizeof(*msg)) {
		if (json) {
			JsonObject & obj = json->createNestedObject(key);
			{
				JsonObject & pool = obj.createNestedObject("pool");
				pool["temp"] = msg->poolTempSetpoint;
				pool["src"] = Utils::strHeatSrc(msg->heatSrc & 0x03);
			} {
				JsonObject & spa = obj.createNestedObject("spa");
				spa["temp"] = msg->spaTempSetpoint;
				spa["src"] = Utils::strHeatSrc(msg->heatSrc >> 2);
			}
		}
		if (sys) {
			sys->pool.setPoint = msg->poolTempSetpoint;
			sys->pool.heatSrc = msg->heatSrc & 0x03;
			sys->spa.setPoint = msg->spaTempSetpoint;
			sys->spa.heatSrc = msg->heatSrc >> 2;
		}
		return true;
	}
	return false;
}

inline bool
decodeCtrl_sched_a5(mCtrlSched_a5_t const * const msg, uint_least8_t const len,
                    sysState_t * sys, JsonObject * json, char const * const key)
{
	if (len == sizeof(*msg)) {
		if (json) {
			JsonObject & obj = json->createNestedObject(key);

			for (uint_least8_t ii = 0; ii < 2; ii++) {
				mCtrlSchedSub_a5_t * p = (mCtrlSchedSub_a5_t *)&msg->sched[ii];
				if (p->circuit) {
					Utils::schedule(obj, p->circuit, (uint16_t)p->prgStartHi << 8 | p->prgStartLo, (uint16_t)p->prgStopHi << 8 | p->prgStopLo);
				}
			}
		}
		if (sys) {
			for (uint_least8_t ii = 0; ii < 2; ii++) {
				mCtrlSchedSub_a5_t * p = (mCtrlSchedSub_a5_t *)&msg->sched[ii];

				uint16_t const start = (uint16_t)p->prgStartHi << 8 | p->prgStartLo;
				uint16_t const stop = (uint16_t)p->prgStopHi << 8 | p->prgStopLo;
				sys->sched[ii].circuit = p->circuit;
				sys->sched[ii].start = start;
				sys->sched[ii].stop = stop;
			}
		}
		return true;
	}
	return false;
}

inline bool
decodeCtrl_layout_a5(mCtrlLayout_a5_t const * const msg, uint_least8_t const len,
                     sysState_t * sys, JsonObject * json, char const * const key)
{
	if (len == sizeof(*msg)) {
		if (json) {
			JsonObject & obj = json->createNestedObject(key);
			JsonArray & array = obj.createNestedArray("layout");
			for (uint_least8_t ii = 0; ii < len; ii++) {
				array.add(Utils::circuitName(msg->circuit[ii]));
			}
		}
		return true;
	}
	return false;
}

inline bool
decodePumpRegulateSet_a5(mPumpRegulateSet_a5_t const * const msg, uint_least8_t const len,
                         sysState_t * sys, JsonObject * json, char const * const key)
{
	if (len == sizeof(*msg)) {
		if (json) {
			JsonObject & obj = json->createNestedObject(key);
			uint16_t const address = (msg->addressHi << 8) | msg->addressLo;
			uint16_t const value = (msg->valueHi << 8) | msg->valueLo;
			obj[Utils::strPumpPrgName(address)] = value;
		}
		return true;
	}
	return false;
}

inline bool
decodePumpRegulateSetAck_a5(mPumpRegulateSetResp_a5_t const * const msg, uint_least8_t const len,
                            sysState_t * sys, JsonObject * json, char const * const key)
{
	if (len == sizeof(*msg)) {
		if (json) {
			JsonObject & obj = json->createNestedObject(key);
			obj["now"] = (msg->valueHi << 8) | msg->valueLo;
		}
		return true;
	}
	return false;
}

inline bool
decodePump_control_a5(mPumpControl_a5_t const * const msg, uint_least8_t const len,
	                  sysState_t * sys, JsonObject * json, char const * const key)
{
	if (len == sizeof(*msg)) {
		if (json) {
			JsonObject & obj = json->createNestedObject(key);
			char const * s;
			switch (msg->control) {
				case 0x00: s = "local"; break;
				case 0xFF: s = "remote"; break;
				default: s = Utils::strHex8(msg->control);
			}
			obj["control"] = s;
		}
		return true;
	}
	return false;
}

inline bool
decodePump_mode_a5(mPumpMode_a5_t const * const msg, uint_least8_t const len,
	               sysState_t * sys, JsonObject * json, char const * const key)
{
	if (len == sizeof(*msg)) {
		if (json) {
			JsonObject & obj = json->createNestedObject(key);
			obj["mode"] = Utils::strPumpMode(msg->mode);
		}
		return true;
	}
	return false;
}

inline bool
decodePump_state_a5(mPumpState_a5_t const * const msg, uint_least8_t const len,
	                sysState_t * sys, JsonObject * json, char const * const key)
{
	if (len == sizeof(*msg) && (msg->state == 0x04 || msg->state == 0x0A)) {
		if (json) {
			JsonObject & obj = json->createNestedObject(key);
			obj["running"] = (msg->state == 0x0A);
		}
		return true;
	}
	return false;
}

inline bool
decodePump_statusReq_a5(struct mPumpStatusReq_a5_t const * const msg, uint_least8_t const len,
	                    sysState_t * sys, JsonObject * json, char const * const key)
{
	if (len == 0) {  // sizeof for some reason returns 1 when struct is empty
		if (json) {
			JsonObject & obj = json->createNestedObject(key);
			(void)obj;
		}
		return true;
	}
	return false;
}

inline bool
decodePump_status_a5(mPumpStatus_a5_t const * const msg, uint_least8_t const len,
                     sysState_t * sys, JsonObject * json, char const * const key)
{
	if (len == sizeof(*msg) && (msg->state == 0x04 || msg->state == 0x0A)) {
		if (json) {
			JsonObject & obj = json->createNestedObject(key);
			obj["running"] = (msg->state == 0x0A);
			obj["mode"] = Utils::strPumpMode(msg->mode);
			obj["status"] = msg->status;
			obj["pwr"] = ((uint16_t)msg->powerHi << 8) | msg->powerLo;
			obj["rpm"] = ((uint16_t)msg->rpmHi << 8) | msg->rpmLo;
			if (msg->gpm) obj["gpm"] = msg->gpm;
			if (msg->pct) obj["pct"] = msg->pct;
			obj["err"] = msg->err;
			obj["timer"] = msg->timer;
			obj["time"] = Utils::strTime(msg->hour, msg->minute);
		}
		if (sys) {
			sys->pump.running = (msg->state == 0x0A);
			sys->pump.mode = msg->mode;
			sys->pump.status = msg->status;
			sys->pump.pwr = ((uint16_t)msg->powerHi << 8) | msg->powerLo;
			sys->pump.rpm = ((uint16_t)msg->rpmHi << 8) | msg->rpmLo;
			sys->pump.err = msg->err;
		}
		return true;
	}
	return false;
}

inline bool
decodeChlor_pingReq_ic(mChlorPingReq_ic_t const * const msg, uint_least8_t const len,
                       sysState_t * sys, JsonObject * json, char const * const key)
{
	if (len == sizeof(*msg)) {
		if (json) {
			JsonObject & obj = json->createNestedObject(key);
			(void)msg;
			(void)obj;
		}
		return true;
	}
	return false;
}

inline bool
decodeChlor_ping_ic(mChlorPing_ic_t const * const msg, uint_least8_t const len,
                    sysState_t * sys, JsonObject * json, char const * const key)
{
	if (len == sizeof(*msg)) {
		if (json) {
			JsonObject & obj = json->createNestedObject(key);
			(void)msg;
			(void)obj;
		}
		return true;
	}
	return false;
}

inline bool
decodeChlor_name_ic(mChlorName_ic_t const * const msg, uint_least8_t const len,
                    sysState_t * sys, JsonObject * json, char const * const key)
{
	if (len == sizeof(*msg)) {
		if (json) {
			JsonObject & obj = json->createNestedObject(key);
			obj["name"] = Utils::strName(msg->name, sizeof(msg->name));
		}
		return true;
	}
	return false;
}

inline bool
decodeChlor_chlorSet_ic(mChlorLvlSet_ic_t const * const msg, uint_least8_t const len,
                        sysState_t * sys, JsonObject * json, char const * const key)
{
	if (len == sizeof(*msg)) {
		if (json) {
			JsonObject & obj = json->createNestedObject(key);
			obj["pct"] = msg->pct;
		}
		if (sys) {
			sys->chlor.pct = msg->pct;
		}
		return true;
	}
	return false;
}


inline bool
decodeChlor_chlorSetResp_ic(mChlorLvlSetResp_ic_t  const * const msg, uint_least8_t const len,
                            sysState_t * sys, JsonObject * json, char const * const key)
{
	if (len == sizeof(*msg)) {
		uint16_t const salt = (uint16_t)msg->salt * 50;
		bool const lowFlow = (msg->err & 0x01);
		if (json) {
			JsonObject & obj = json->createNestedObject(key);
			obj["salt"] = salt;
			obj["flow"] = !lowFlow;
		}
		if (sys) {
			sys->chlor.salt = salt;
			if (salt < 2600) {
				sys->chlor.state = CHLORSTATE_veryLowSalt;
			}
			else if (salt < 2800) {
				sys->chlor.state = CHLORSTATE_lowSalt;
			}
			else if (salt > 4500) {
				sys->chlor.state = CHLORSTATE_highSalt;
			}
			else if (lowFlow) {
				sys->chlor.state = CHLORSTATE_lowFlow;
			}
			else {
				sys->chlor.state = CHLORSTATE_ok;
			}
		}
		return true;
	}
	return false;
}

static bool
_decodeA5_ctrl(pentairMsg_t * msg, sysState_t * sys, JsonObject * json)
{
	bool found;
	bool decoded = false;
	DATALINK_A5_CTRL_MSGTYP_t const mt = (DATALINK_A5_CTRL_MSGTYP_t)msg->hdr.typ;
	char const * s = Utils::mtCtrlName(mt, &found);

	if (found) {
		switch (mt) {

			// response to various set requests
			case DATALINK_A5_CTRL_MSGTYP_setAck:
				decoded = decodeCtrl_setAck_a5((mCtrlSetAck_a5_t *) msg->data, msg->hdr.len, sys, json, s);
				break;

				// set circuit request (there appears to be no separate "get circuit request")
			case DATALINK_A5_CTRL_MSGTYP_circuitSet:
				decoded = decodeCtrl_circuitSet_a5((mCtrlCircuitSet_a5_t *) msg->data, msg->hdr.len, sys, json, s);
				break;

				// various get requests
			case DATALINK_A5_CTRL_MSGTYP_timeReq:
			case DATALINK_A5_CTRL_MSGTYP_stateReq:
			case DATALINK_A5_CTRL_MSGTYP_heatReq:
			case DATALINK_A5_CTRL_MSGTYP_schedReq:
#if 0
			case DATALINK_A5_CTRL_MSGTYP_layoutReq:
#endif
				decoded = decodeCtrl_variousReq_a5(msg->data, msg->hdr.len, sys, json, s);
				break;

				// schedule: get response / set request
			case DATALINK_A5_CTRL_MSGTYP_sched:
				//case DATALINK_A5_CTRL_MSGTYP_schedSet:
				decoded = decodeCtrl_sched_a5((mCtrlSched_a5_t *) msg->data, msg->hdr.len, sys, json, s);
				break;

				// state: get response / set request
			case DATALINK_A5_CTRL_MSGTYP_state:
				decoded = decodeCtrl_state_a5((mCtrlState_a5_t *)msg->data, msg->hdr.len, sys, json, s);
				break;
			case DATALINK_A5_CTRL_MSGTYP_stateSet:
				// 2BD
				break;

				// time: get response / set request
			case DATALINK_A5_CTRL_MSGTYP_time:
			case DATALINK_A5_CTRL_MSGTYP_timeSet:
				decoded = decodeCtrl_time_a5((mCtrlTime_a5_t *)msg->data, msg->hdr.len, sys, json, s);
				break;

				// heat: get response / set request
			case DATALINK_A5_CTRL_MSGTYP_heat:
				decoded = decodeCtrl_heat_a5((mCtrlHeat_a5_t *)msg->data, msg->hdr.len, sys, json, s);
				break;
			case DATALINK_A5_CTRL_MSGTYP_heatSet:
				decoded = decodeCtrl_heatSet_a5((mCtrlHeatSet_a5_t *)msg->data, msg->hdr.len, sys, json, s);
				break;

#if 0
				// layout: get response / set request
			case DATALINK_A5_CTRL_MSGTYP_layout:
			case DATALINK_A5_CTRL_MSGTYP_layoutSet:
				decoded = decodeCtrl_layout_a5(json, sys, s, hdr->len, (mCtrlLayout_a5_t *)data);
				break;
#endif
		}
	}
	return decoded;
};

static bool
_decodeA5_pump(pentairMsg_t * msg, sysState_t * sys, JsonObject * json)
{
	bool found;
	bool decoded = false;
	bool toPump = (Utils::addrGroup(msg->hdr.dst) == ADDRGROUP_pump);
	DATALINK_A5_PUMP_MSGTYP_t const mt = (DATALINK_A5_PUMP_MSGTYP_t)msg->hdr.typ;
	char const * s = Utils::mtPumpName(mt, toPump, &found);

	if (found) {
		switch (mt) {

			case DATALINK_A5_PUMP_MSGTYP_regulate:
				decoded = toPump
				    ? decodePumpRegulateSet_a5((mPumpRegulateSet_a5_t *) msg->data, msg->hdr.len, sys, json, s)
					: decodePumpRegulateSetAck_a5((mPumpRegulateSetResp_a5_t *) msg->data, msg->hdr.len, sys, json, s);
				break;

			case DATALINK_A5_PUMP_MSGTYP_control:
				decoded = decodePump_control_a5((mPumpControl_a5_t *) msg->data, msg->hdr.len, sys, json, s);
				break;

			case DATALINK_A5_PUMP_MSGTYP_mode:
				decoded = decodePump_mode_a5((mPumpMode_a5_t *) msg->data, msg->hdr.len, sys, json, s);
				break;

			case DATALINK_A5_PUMP_MSGTYP_state:
				decoded = decodePump_state_a5((mPumpState_a5_t *) msg->data, msg->hdr.len, sys, json, s);
				break;

			case DATALINK_A5_PUMP_MSGTYP_status:
				decoded = toPump
				    ? decodePump_statusReq_a5((mPumpStatusReq_a5_t *) msg->data, msg->hdr.len, sys, json, s)
					: decodePump_status_a5((mPumpStatus_a5_t *) msg->data, msg->hdr.len, sys, json, s);
				break;

			case DATALINK_A5_PUMP_MSGTYP_0xFF:
				decoded = true;  // silently ignore
				break;
		}
	}
	return decoded;
}

static bool
_decodeIC_chlor(pentairMsg_t * msg, sysState_t * sys, JsonObject * json)
{
	bool found;
	bool decoded = false;
	DATALINK_IC_CHLOR_MSGTYP_t const mt = (DATALINK_IC_CHLOR_MSGTYP_t)msg->hdr.typ;
	char const * s = Utils::mtChlorName(mt, &found);

	if (found) {
		switch (msg->hdr.typ) {

			case DATALINK_IC_CHLOR_MSGTYP_pingReq:
				decoded = decodeChlor_pingReq_ic((mChlorPingReq_ic_t *) msg->data, msg->hdr.len, sys, json, s);
				break;

			case DATALINK_IC_CHLOR_MSGTYP_ping:
				decoded = decodeChlor_ping_ic((mChlorPing_ic_t *) msg->data, msg->hdr.len, sys, json, s);
				break;

			case DATALINK_IC_CHLOR_MSGTYP_name:  // name
				decoded = decodeChlor_name_ic((mChlorName_ic_t *) msg->data, msg->hdr.len, sys, json, s);
				break;

			case DATALINK_IC_CHLOR_MSGTYP_lvlSet:
				decoded = decodeChlor_chlorSet_ic((mChlorLvlSet_ic_t *) msg->data, msg->hdr.len, sys, json, s);
				break;

			case DATALINK_IC_CHLOR_MSGTYP_lvlSetResp:
				decoded = decodeChlor_chlorSetResp_ic((mChlorLvlSetResp_ic_t  *) msg->data, msg->hdr.len, sys, json, s);
				break;

			case DATALINK_IC_CHLOR_MSGTYP_0x14:  // ?? (len == 1, data[0] == 0x00)gr
				decoded = true;  // stick our head in the sand
				break;
		}
	}
	return decoded;
}

/**
 * @brief Decode Pentair RS-485 message; update the system state and returns the decoded message as JSON
 *
 * @param root   JSON object to store the decoded message
 * @param sys    system state, to be updated with decoded message
 * @param proto  Pentair packet protocol version
 * @param hdr    Pentair packet header
 * @param data   Pentair packet data
 */
void
Decode::decode(pentairMsg_t * msg, sysState_t * sys, JsonObject * root)
{
	bool decoded = false;
	addrGroup_t src = Utils::addrGroup(msg->hdr.src);
	addrGroup_t dst = Utils::addrGroup(msg->hdr.dst);

	if (dst == ADDRGROUP_unused9) {
		return; // stick our head in the sand
	}

	Utils::resetIdx();

	switch (msg->proto) {
		case PROTOCOL_a5:
			if (src == ADDRGROUP_pump || dst == ADDRGROUP_pump) {
				if (root && CONFIG_POOL_DBG_DECODED_PUMP) {
					JsonObject & json = root->createNestedObject("pump");
					decoded = _decodeA5_pump(msg, sys, &json);
				} else {
					decoded = _decodeA5_pump(msg, sys, NULL);
				}
			}
			else {
				if (root && CONFIG_POOL_DBG_DECODED_CTRL) {
					JsonObject & json = root->createNestedObject("ctrl");
					decoded = _decodeA5_ctrl(msg, sys, &json);
				} else {
					decoded = _decodeA5_ctrl(msg, sys, NULL);
				}
			}
			break;
		case PROTOCOL_ic:
			if (dst == ADDRGROUP_all || dst == ADDRGROUP_chlor) {
				if (root  && CONFIG_POOL_DBG_DECODED_CHLORINATOR) {
					JsonObject & json = root->createNestedObject("chlo");
					decoded = _decodeIC_chlor(msg, sys, &json);
				} else {
					decoded = _decodeIC_chlor(msg, sys, NULL);
				}
			}
			break;
	}

	if (!decoded) {
		Utils::jsonRaw(msg, root, "raw");
	}
}

