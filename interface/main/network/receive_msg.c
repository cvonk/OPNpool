/**
 * @brief Decode network messages received over the rs-485 bus
 *
 * CLOSED SOURCE, NOT FOR PUBLIC RELEASE
 * (c) Copyright 2015 - 2020, Coert Vonk
 * All rights reserved. Use of copyright notice does not imply publication.
 * All text above must be included in any redistribution
 **/

#include <esp_system.h>
#include <esp_log.h>

//#include "pooltypes.h"
//#include "poolstate.h"
//#include "utils.h"
#include "../datalink/datalink.h"
#include "network.h"
#include "name.h"

//static char const * const TAG = "decode";

#if 0

inline bool
decodeCtrl_setAck_a5(mCtrlSetAck_a5_t * datalink, uint datalink_len, network_msg_t * network)
{
	(void) network;
    return datalink_len == sizeof(*datalink);
}

inline bool
decodeCtrl_circuitSet_a5(mCtrlCircuitSet_a5_t * datalink, uint datalink_len, network_msg_t * network)
{
	(void) network;
	printf("decodeCtrl_circuitSet_a5 datalink=(%02X %02X)\n", datalink->circuit, datalink->value);
	if (datalink_len == sizeof(*datalink)) {
		if (json) {
			JsonObject & obj = json->createNestedObject(key);
			obj[Utils::circuitName(datalink->circuit)] = datalink->value;
		}
		return true;
	}
	return false;
}

inline bool
decodeCtrl_variousReq_a5(uint8_t * datalink, uint datalink_len,
                         network_msg_t * network)
{
	(void) network;
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
decodeCtrl_time_a5(mCtrlTime_a5_t const * const datalink, uint datalink_len,
                   network_msg_t * network)
{
	if (datalink_len == sizeof(*datalink)) {
		if (json) {
			JsonObject & obj = json->createNestedObject(key);
			obj["time"] = Utils::strTime(datalink->hour, datalink->minute);
			obj["date"] = Utils::strDate(datalink->year, datalink->month, datalink->day);
			//obj["dst"] = datalink->daylightSavings;
			//int16_t cs = datalink->clkSpeed;
			//obj["clkSpeed"] = min( cs, 60 ) * 5;  // adjustment unit=5
		}
		if (network) {
			network->time.minute = datalink->minute;
			network->time.hour = datalink->hour;
			network->date.day = datalink->day;
			network->date.month = datalink->month;
			network->date.year = datalink->year;
		}
		return true;
	}
	return false;
}

inline bool
decodeCtrl_state_a5(mCtrlState_a5_t const * const datalink, uint datalink_len,
                    network_msg_t * network)
{
	if (datalink_len == sizeof(*datalink)) {
		if (json) {
			JsonObject & obj = json->createNestedObject(key);
			{
				JsonObject & system = obj.createNestedObject("system");
				system["time"] = Utils::strTime(datalink->hour, datalink->minute);
				system["fw"] = datalink->major + (float)datalink->minor / 100;
			} {
				Utils::activeCircuits(obj, "active", (datalink->activeHi << 8) | datalink->activeLo);
				Utils::activeCircuits(obj, "delay", datalink->delayLo);
			} {
				JsonObject & pool = obj.createNestedObject("pool");
				pool["temp"] = datalink->poolTemp;
				pool["src"] = Utils::strHeatSrc(datalink->heatSrc & 0x03);
				pool["heating"] = (bool)(datalink->heating & 0x04);
			} {
				JsonObject & spa = obj.createNestedObject("spa");
				spa["temp"] = datalink->spaTemp;
				spa["src"] = Utils::strHeatSrc(datalink->heatSrc >> 2);
				spa["heating"] = (bool)(datalink->heating & 0x08);
			} {
				JsonObject & temp = obj.createNestedObject("temp");
				temp["air"] = datalink->airtemp;
				if (datalink->solartemp != 0xFF) {
					temp["solar"] = datalink->solartemp;
				}
			}
		}
		if (network) {
			network->time.minute = datalink->minute;
			network->time.hour = datalink->hour;
			network->circuits.active = (datalink->activeHi << 8) | datalink->activeLo;
			network->circuits.delay = datalink->delayLo;
			network->pool.temp = datalink->poolTemp;
			network->pool.heatSrc = datalink->heatSrc & 0x03;
			network->pool.heating = datalink->heating & 0x04;
			network->spa.temp = datalink->poolTemp;
			network->spa.heatSrc = datalink->heatSrc >> 2;
			network->spa.heating = datalink->heating & 0x08;
			network->air.temp = datalink->airtemp;
		}
		return true;
	}
	return false;
}

inline bool
decodeCtrl_heat_a5(mCtrlHeat_a5_t const * const datalink, uint datalink_len,
                   network_msg_t * network)
{
	if (datalink_len == sizeof(*datalink)) {
		if (json) {
			JsonObject & obj = json->createNestedObject(key);
			{
				JsonObject & pool = obj.createNestedObject("pool");
				pool["temp"] = datalink->poolTemp;
				pool["sp"] = datalink->poolTempSetpoint;
				pool["src"] = Utils::strHeatSrc(datalink->heatSrc & 0x03);
			} {
				JsonObject & spa = obj.createNestedObject("spa");
				spa["temp"] = datalink->spaTemp;
				spa["sp"] = datalink->spaTempSetpoint;
				spa["src"] = Utils::strHeatSrc(datalink->heatSrc >> 2);
			}
		}
		if (network) {
			network->pool.setPoint = datalink->poolTempSetpoint;
			network->spa.setPoint = datalink->spaTempSetpoint;
		}
		return true;
	}
	return false;
}

inline bool
decodeCtrl_heatSet_a5(mCtrlHeatSet_a5_t const * const datalink, uint datalink_len,
                      network_msg_t * network)
{
	if (datalink_len == sizeof(*datalink)) {
		if (json) {
			JsonObject & obj = json->createNestedObject(key);
			{
				JsonObject & pool = obj.createNestedObject("pool");
				pool["temp"] = datalink->poolTempSetpoint;
				pool["src"] = Utils::strHeatSrc(datalink->heatSrc & 0x03);
			} {
				JsonObject & spa = obj.createNestedObject("spa");
				spa["temp"] = datalink->spaTempSetpoint;
				spa["src"] = Utils::strHeatSrc(datalink->heatSrc >> 2);
			}
		}
		if (network) {
			network->pool.setPoint = datalink->poolTempSetpoint;
			network->pool.heatSrc = datalink->heatSrc & 0x03;
			network->spa.setPoint = datalink->spaTempSetpoint;
			network->spa.heatSrc = datalink->heatSrc >> 2;
		}
		return true;
	}
	return false;
}

inline bool
decodeCtrl_sched_a5(mCtrlSched_a5_t const * const datalink, uint datalink_len,
                    network_msg_t * network)
{
	if (datalink_len == sizeof(*datalink)) {
		if (json) {
			JsonObject & obj = json->createNestedObject(key);

			for (uint_least8_t ii = 0; ii < 2; ii++) {
				mCtrlSchedSub_a5_t * p = (mCtrlSchedSub_a5_t *)&datalink->sched[ii];
				if (p->circuit) {
					Utils::schedule(obj, p->circuit, (uint16_t)p->prgStartHi << 8 | p->prgStartLo, (uint16_t)p->prgStopHi << 8 | p->prgStopLo);
				}
			}
		}
		if (network) {
			for (uint_least8_t ii = 0; ii < 2; ii++) {
				mCtrlSchedSub_a5_t * p = (mCtrlSchedSub_a5_t *)&datalink->sched[ii];

				uint16_t const start = (uint16_t)p->prgStartHi << 8 | p->prgStartLo;
				uint16_t const stop = (uint16_t)p->prgStopHi << 8 | p->prgStopLo;
				network->sched[ii].circuit = p->circuit;
				network->sched[ii].start = start;
				network->sched[ii].stop = stop;
			}
		}
		return true;
	}
	return false;
}

inline bool
decodeCtrl_layout_a5(mCtrlLayout_a5_t const * const datalink, uint datalink_len,
                     network_msg_t * network)
{
	if (datalink_len == sizeof(*datalink)) {
		if (json) {
			JsonObject & obj = json->createNestedObject(key);
			JsonArray & array = obj.createNestedArray("layout");
			for (uint_least8_t ii = 0; ii < len; ii++) {
				array.add(Utils::circuitName(datalink->circuit[ii]));
			}
		}
		return true;
	}
	return false;
}

inline bool
decodePumpRegulateSet_a5(mPumpRegulateSet_a5_t const * const datalink, uint datalink_len,
                         network_msg_t * network)
{
	if (datalink_len == sizeof(*datalink)) {
        *network = {
            .typ = NETWORK_MSGTYP_PUMP_REG_SET,
            .pump_regulate_set = {
                .prg_name = name_pump_prg((datalink->addressHi << 8) | datalink->addressLo),
                .value = (datalink->valueHi << 8) | datalink->valueLo,
            }
        }
		return true;
	}
	return false;
}

inline bool
decodePumpRegulateSetAck_a5(mPumpRegulateSetResp_a5_t const * const datalink, uint datalink_len,
                            network_msg_t * network)
{
	if (datalink_len == sizeof(*datalink)) {
		if (json) {
			JsonObject & obj = json->createNestedObject(key);
			obj["now"] = (datalink->valueHi << 8) | datalink->valueLo;
		}
		return true;
	}
	return false;
}

inline bool
decodePump_control_a5(mPumpCtrl_a5_t const * const datalink, uint datalink_len,
	                  network_msg_t * network)
{
	if (datalink_len == sizeof(*datalink)) {
		if (json) {
			JsonObject & obj = json->createNestedObject(key);
			char const * s;
			switch (datalink->control) {
				case 0x00: s = "local"; break;
				case 0xFF: s = "remote"; break;
				default: s = Utils::strHex8(datalink->control);
			}
			obj["control"] = s;
		}
		return true;
	}
	return false;
}

inline bool
decodePump_mode_a5(mPumpMode_a5_t const * const datalink, uint datalink_len,
	               network_msg_t * network)
{
	if (datalink_len == sizeof(*datalink)) {
		if (json) {
			JsonObject & obj = json->createNestedObject(key);
			obj["mode"] = Utils::strPumpMode(datalink->mode);
		}
		return true;
	}
	return false;
}

inline bool
decodePump_state_a5(mPumpState_a5_t const * const datalink, uint datalink_len,
	                network_msg_t * network)
{
	if (datalink_len == sizeof(*datalink) && (datalink->state == 0x04 || datalink->state == 0x0A)) {
		if (json) {
			JsonObject & obj = json->createNestedObject(key);
			obj["running"] = (datalink->state == 0x0A);
		}
		return true;
	}
	return false;
}

inline bool
decodePump_statusReq_a5(struct mPumpStatusReq_a5_t const * const datalink, uint datalink_len,
	                    network_msg_t * network)
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
decodePump_status_a5(mPumpStatus_a5_t const * const datalink, uint datalink_len,
                     network_msg_t * network)
{
	if (datalink_len == sizeof(*datalink) && (datalink->state == 0x04 || datalink->state == 0x0A)) {
		if (json) {
			JsonObject & obj = json->createNestedObject(key);
			obj["running"] = (datalink->state == 0x0A);
			obj["mode"] = Utils::strPumpMode(datalink->mode);
			obj["status"] = datalink->status;
			obj["pwr"] = ((uint16_t)datalink->powerHi << 8) | datalink->powerLo;
			obj["rpm"] = ((uint16_t)datalink->rpmHi << 8) | datalink->rpmLo;
			if (datalink->gpm) obj["gpm"] = datalink->gpm;
			if (datalink->pct) obj["pct"] = datalink->pct;
			obj["err"] = datalink->err;
			obj["timer"] = datalink->timer;
			obj["time"] = Utils::strTime(datalink->hour, datalink->minute);
		}
		if (network) {
			network->pump.running = (datalink->state == 0x0A);
			network->pump.mode = datalink->mode;
			network->pump.status = datalink->status;
			network->pump.pwr = ((uint16_t)datalink->powerHi << 8) | datalink->powerLo;
			network->pump.rpm = ((uint16_t)datalink->rpmHi << 8) | datalink->rpmLo;
			network->pump.err = datalink->err;
		}
		return true;
	}
	return false;
}

inline bool
decodeChlor_pingReq_ic(mChlorPingReq_ic_t const * const datalink, uint datalink_len,
                       network_msg_t * network)
{
	if (datalink_len == sizeof(*datalink)) {
		if (json) {
			JsonObject & obj = json->createNestedObject(key);
			(void)datalink;
			(void)obj;
		}
		return true;
	}
	return false;
}

inline bool
decodeChlor_ping_ic(mChlorPing_ic_t const * const datalink, uint datalink_len,
                    network_msg_t * network)
{
	if (datalink_len == sizeof(*datalink)) {
		if (json) {
			JsonObject & obj = json->createNestedObject(key);
			(void)datalink;
			(void)obj;
		}
		return true;
	}
	return false;
}

inline bool
decodeChlor_name_ic(mChlorName_ic_t const * const datalink, uint datalink_len,
                    network_msg_t * network)
{
	if (datalink_len == sizeof(*datalink)) {
		if (json) {
			JsonObject & obj = json->createNestedObject(key);
			obj["name"] = Utils::strName(datalink->name, sizeof(datalink->name));
		}
		return true;
	}
	return false;
}

inline bool
decodeChlor_chlorSet_ic(mChlorLevelSet_ic_t const * const datalink, uint datalink_len,
                        network_msg_t * network)
{
	if (datalink_len == sizeof(*datalink)) {
		if (json) {
			JsonObject & obj = json->createNestedObject(key);
			obj["pct"] = datalink->pct;
		}
		if (network) {
			network->chlor.pct = datalink->pct;
		}
		return true;
	}
	return false;
}


inline bool
decodeChlor_chlorSetResp_ic(mChlorLvlSetResp_ic_t  const * const datalink, uint datalink_len,
                            network_msg_t * network)
{
	if (datalink_len == sizeof(*datalink)) {
		uint16_t const salt = (uint16_t)datalink->salt * 50;
		bool const lowFlow = (datalink->err & 0x01);
		if (json) {
			JsonObject & obj = json->createNestedObject(key);
			obj["salt"] = salt;
			obj["flow"] = !lowFlow;
		}
		if (network) {
			network->chlor.salt = salt;
			if (salt < 2600) {
				network->chlor.state = CHLORSTATE_veryLowSalt;
			}
			else if (salt < 2800) {
				network->chlor.state = CHLORSTATE_lowSalt;
			}
			else if (salt > 4500) {
				network->chlor.state = CHLORSTATE_highSalt;
			}
			else if (lowFlow) {
				network->chlor.state = CHLORSTATE_lowFlow;
			}
			else {
				network->chlor.state = CHLORSTATE_ok;
			}
		}
		return true;
	}
	return false;
}
#endif

static void
_decodeA5_ctrl(datalink_pkt_t const * const datalink, network_msg_t * const network)
{
	bool found;
	MT_CTRL_A5_t const mt = (MT_CTRL_A5_t) datalink->hdr.typ;
	(void)name_mtCtrl(mt, &found);

    network->typ = NETWORK_MSGTYP_NONE;

    if (found) {
        switch (mt) {

			// response to various set requests
			case MT_CTRL_SET_ACK:
                if (datalink->hdr.len == sizeof(mCtrlSetAck_a5_t)) {
                    network->typ = NETWORK_MSGTYP_CTRL_SET_ACK;
                    network->u.ctrl_set_ack = (mCtrlSetAck_a5_t *) datalink->data;
                }
                break;

            // set circuit request (there appears to be no separate "get circuit request")
			case MT_CTRL_CIRCUIT_SET:
                if (datalink->hdr.len == sizeof(mCtrlCircuitSet_a5_t)) {
                    network->typ = NETWORK_MSGTYP_CTRL_CIRCUIT_SET;
                    network->u.ctrl_circuit_set = (mCtrlCircuitSet_a5_t *) datalink->data;
                }
                break;

				// various get requests
			case MT_CTRL_TIME_REQ:
                if (datalink->hdr.len == 0) {
                    network->typ = NETWORK_MSGTYP_CTRL_TIME_REQ;
                }
                break;
			case MT_CTRL_STATE_REQ:
                if (datalink->hdr.len == 0) {
                    network->typ = NETWORK_MSGTYP_CTRL_STATE_REQ;
                }
                break;
			case MT_CTRL_HEAT_REQ:
                if (datalink->hdr.len == 0) {
                    network->typ = NETWORK_MSGTYP_CTRL_HEAT_REQ;
                }
                break;
			case MT_CTRL_SCHED_REQ:
                if (datalink->hdr.len == 0) {
                    network->typ = NETWORK_MSGTYP_CTRL_SCHED_REQ;
                }
                break;
			case MT_CTRL_LAYOUT_REQ:
                if (datalink->hdr.len == 0) {
                    network->typ = NETWORK_MSGTYP_CTRL_LAYOUT_REQ;
                }
                break;

				// schedule: get response / set request
			case MT_CTRL_SCHED:
                if (datalink->hdr.len == sizeof(mCtrlSched_a5_t)) {
                    network->typ = NETWORK_MSGTYP_CTRL_SCHED;
                    network->u.ctrl_sched = (mCtrlSched_a5_t *) datalink->data;
                }
                break;

				// state: get response / set request
			case MT_CTRL_STATE:
                if (datalink->hdr.len == sizeof(mCtrlState_a5_t)) {
                    network->typ = NETWORK_MSGTYP_CTRL_STATE;
                    network->u.ctrl_state = (mCtrlState_a5_t *) datalink->data;
                }
                break;
			case MT_CTRL_STATE_SET:
                // unfinished
                //if (datalink->hdr.len == sizeof(mCtrlState_a5_t)) {
                    network->typ = NETWORK_MSGTYP_CTRL_STATE_SET;
                    //network->u.ctrl_state = (mCtrlState_a5_t *) datalink->data;
                //}
				break;

				// time: get response / set request
			case MT_CTRL_TIME:
                if (datalink->hdr.len == sizeof(mCtrlTime_a5_t)) {
                    network->typ = NETWORK_MSGTYP_CTRL_TIME;
                    network->u.ctrl_time = (mCtrlTime_a5_t *) datalink->data;
                }
                break;
			case MT_CTRL_TIME_SET:
                if (datalink->hdr.len == sizeof(mCtrlTimeSet_a5_t)) {
                    network->typ = NETWORK_MSGTYP_CTRL_TIME_SET;
                    network->u.ctrl_time_set = (mCtrlTimeSet_a5_t *) datalink->data;
                }
                break;

				// heat: get response / set request
			case MT_CTRL_HEAT:
                if (datalink->hdr.len == sizeof(mCtrlHeat_a5_t)) {
                    network->typ = NETWORK_MSGTYP_CTRL_HEAT;
                    network->u.ctrl_heat = (mCtrlHeat_a5_t *) datalink->data;
                }
				break;
			case MT_CTRL_HEAT_SET:
                if (datalink->hdr.len == sizeof(mCtrlHeatSet_a5_t)) {
                    network->typ = NETWORK_MSGTYP_CTRL_HEAT_SET;
                    network->u.ctrl_heat_set = (mCtrlHeatSet_a5_t *) datalink->data;
                }
				break;
			case MT_CTRL_LAYOUT:
                if (datalink->hdr.len == sizeof(mCtrlLayout_a5_t)) {
                    network->typ = NETWORK_MSGTYP_CTRL_LAYOUT;
                    network->u.ctrl_layout = (mCtrlLayout_a5_t *) datalink->data;
                }
				break;
			case MT_CTRL_LAYOUT_SET:
                if (datalink->hdr.len == sizeof(mCtrlLayoutSet_a5_t)) {
                    network->typ = NETWORK_MSGTYP_CTRL_LAYOUT_SET;
                    network->u.ctrl_layout_set = (mCtrlLayoutSet_a5_t *) datalink->data;
                }
				break;
		}
	}
};

static void
_decodeA5_pump(datalink_pkt_t const * const datalink, network_msg_t * const network)
{
	bool toPump = (network_group_addr(datalink->hdr.dst) == NETWORK_ADDRGROUP_PUMP);
	MT_PUMP_A5_t const mt = (MT_PUMP_A5_t)datalink->hdr.typ;
	bool found;
	(void)name_mtPump(mt, toPump, &found);

    network->typ = NETWORK_MSGTYP_NONE;

    if (found) {
        switch (mt) {
			case MT_PUMP_REGULATE:
                if (toPump) {
                    if (datalink->hdr.len == sizeof(mPumpRegulateSet_a5_t)) {
                        network->typ = NETWORK_MSGTYP_PUMP_REG_SET;
                        network->u.pump_reg_set = (mPumpRegulateSet_a5_t *) datalink->data;
                    }
                } else {
                    if (datalink->hdr.len == sizeof(mPumpRegulateSetResp_a5_t)) {
                        network->typ = NETWORK_MSGTYP_PUMP_REG_SET_RESP;
                        network->u.pump_reg_set_resp = (mPumpRegulateSetResp_a5_t *) datalink->data;
                    }
                }
                break;
			case MT_PUMP_CTRL:
                if (datalink->hdr.len == sizeof(mPumpCtrl_a5_t)) {
                    network->typ = NETWORK_MSGTYP_PUMP_REG_SET_RESP;
                    network->u.pump_ctrl = (mPumpCtrl_a5_t *) datalink->data;
                }
                break;
			case MT_PUMP_MODE:
                if (datalink->hdr.len == sizeof(mPumpMode_a5_t)) {
                    network->typ = NETWORK_MSGTYP_PUMP_MODE;
                    network->u.pump_mode = (mPumpMode_a5_t *) datalink->data;
                }
                break;
			case MT_PUMP_STATE:
                if (datalink->hdr.len == sizeof(mPumpState_a5_t)) {
                    network->typ = NETWORK_MSGTYP_PUMP_STATE;
                    network->u.pump_state = (mPumpState_a5_t *) datalink->data;
                }
                break;
			case MT_PUMP_STATUS:
                if (toPump) {
                    if (datalink->hdr.len == 0) {
                        network->typ = NETWORK_MSGTYP_PUMP_STATUS_REQ;
                    }
                } else {
                    if (datalink->hdr.len == sizeof(mPumpStatus_a5_t)) {
                        network->typ = NETWORK_MSGTYP_PUMP_STATUS;
                        network->u.pump_status = (mPumpStatus_a5_t *) datalink->data;
                    }
                }
                break;
			case MT_PUMP_0xFF:
                // silently ignore
                break;
        }
    }
}

static void
_decodeIC_chlor(datalink_pkt_t const * const datalink, network_msg_t * const network)
{
	bool found;
	MT_CHLOR_IC_t const mt = (MT_CHLOR_IC_t) datalink->hdr.typ;
	(void)name_mtChlor(mt, &found);

    network->typ = NETWORK_MSGTYP_NONE;

	if (found) {
		switch (datalink->hdr.typ) {
			case MT_CHLOR_PING_REQ:
                if (datalink->hdr.len == sizeof(mChlorPingReq_ic_t)) {
                    network->typ = NETWORK_MSGTYP_CHLOR_PING_REQ;
                    network->u.chlor_ping_req = (mChlorPingReq_ic_t *) datalink->data;
                }
                break;
			case MT_CHLOR_PING:
                if (datalink->hdr.len == sizeof(mChlorPing_ic_t)) {
                    network->typ = NETWORK_MSGTYP_CHLOR_PING;
                    network->u.chlor_ping = (mChlorPing_ic_t *) datalink->data;
                }
                break;
			case MT_CHLOR_NAME:
                if (datalink->hdr.len == sizeof(mChlorName_ic_t)) {
                    network->typ = NETWORK_MSGTYP_CHLOR_NAME;
                    network->u.chlor_name = (mChlorName_ic_t *) datalink->data;
                }
                break;
            case MT_CHLOR_LEVEL_SET:
                if (datalink->hdr.len == sizeof(mChlorLevelSet_ic_t)) {
                    network->typ = NETWORK_MSGTYP_CHLOR_LEVEL_SET;
                    network->u.chlor_level_set = (mChlorLevelSet_ic_t *) datalink->data;
                }
				break;
			case MT_CHLOR_LEVEL_RESP:
                if (datalink->hdr.len == sizeof(mChlorLevelResp_ic_t)) {
                    network->typ = NETWORK_MSGTYP_CHLOR_LEVEL_RESP;
                    network->u.chlor_level_resp = (mChlorLevelResp_ic_t *) datalink->data;
                }
				break;
			case MT_CHLOR_0x14:  // ?? (len == 1, data[0] == 0x00)gr
                // silently ignore
				break;
		}
	}
}

/**
 * @brief Decode Pentair RS-485 message; update the system state and returns the decoded message as JSON
 *
 * @param root   JSON object to store the decoded message
 * @param network    system state, to be updated with decoded message
 * @param proto  Pentair packet protocol version
 * @param hdr    Pentair packet header
 * @param data   Pentair packet data
 */

bool
network_receive_msg(datalink_pkt_t const * const datalink, network_msg_t * const network, bool * const txOpportunity)
{
	NETWORK_ADDRGROUP_t src = network_group_addr(datalink->hdr.src);
	NETWORK_ADDRGROUP_t dst = network_group_addr(datalink->hdr.dst);

	if (dst == NETWORK_ADDRGROUP_UNUSED9) {
		return false; // stick our head in the sand
	}

	name_reset_idx();

	switch (datalink->proto) {
		case NETWORK_PROT_A5:
			if (src == NETWORK_ADDRGROUP_PUMP || dst == NETWORK_ADDRGROUP_PUMP) {
                _decodeA5_pump(datalink, network);
			} else {
                _decodeA5_ctrl(datalink, network);
			}
			break;
		case NETWORK_PROT_IC:
			if (dst == NETWORK_ADDRGROUP_ALL || dst == NETWORK_ADDRGROUP_CHLOR) {
                _decodeIC_chlor(datalink, network);
			}
			break;
	}
    *txOpportunity = (datalink->proto == NETWORK_PROT_A5) &&
                    (network_group_addr(datalink->hdr.src) == NETWORK_ADDRGROUP_CTRL) &&
                    (network_group_addr(datalink->hdr.dst) == NETWORK_ADDRGROUP_ALL);

    return network->typ != NETWORK_MSGTYP_NONE;
}
