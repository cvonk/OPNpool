#pragma once
#include <esp_system.h>

#include "../datalink/datalink.h"
#include "../network/network.h"
#include "../network/network_msg.h"

#define MSG_TYP_MAP(XX) \
  XX( 0, NONE,             network_msg_none_t,             0,                     0)                             \
  XX( 1, CTRL_SET_ACK,     network_msg_ctrl_set_ack_t,     DATALINK_PROT_A5_CTRL, NETWORK_TYP_CTRL_SET_ACK)     \
  XX( 2, CTRL_CIRCUIT_SET, network_msg_ctrl_circuit_set_t, DATALINK_PROT_A5_CTRL, NETWORK_TYP_CTRL_CIRCUIT_SET) \
  XX( 3, CTRL_SCHED_REQ,   network_msg_ctrl_sched_req_t,   DATALINK_PROT_A5_CTRL, NETWORK_TYP_CTRL_SCHED_REQ)   \
  XX( 4, CTRL_SCHED_RESP,  network_msg_ctrl_sched_resp_t,  DATALINK_PROT_A5_CTRL, NETWORK_TYP_CTRL_SCHED)       \
  XX( 5, CTRL_STATE_REQ,   network_msg_ctrl_state_req_t,   DATALINK_PROT_A5_CTRL, NETWORK_TYP_CTRL_STATE_REQ)   \
  XX( 6, CTRL_STATE,       network_msg_ctrl_state_t,       DATALINK_PROT_A5_CTRL, NETWORK_TYP_CTRL_STATE)       \
  XX( 7, CTRL_STATE_SET,   network_msg_ctrl_state_set_t,   DATALINK_PROT_A5_CTRL, NETWORK_TYP_CTRL_STATE_SET)   \
  XX( 8, CTRL_TIME_REQ,    network_msg_ctrl_time_req_t,    DATALINK_PROT_A5_CTRL, NETWORK_TYP_CTRL_TIME_REQ)    \
  XX( 9, CTRL_TIME,        network_msg_ctrl_time_t,        DATALINK_PROT_A5_CTRL, NETWORK_TYP_CTRL_TIME)        \
  XX(10, CTRL_TIME_SET,    network_msg_ctrl_time_t,        DATALINK_PROT_A5_CTRL, NETWORK_TYP_CTRL_TIME_SET)    \
  XX(11, CTRL_HEAT_REQ,    network_msg_ctrl_heat_req_t,    DATALINK_PROT_A5_CTRL, NETWORK_TYP_CTRL_HEAT_REQ)    \
  XX(12, CTRL_HEAT,        network_msg_ctrl_heat_t,        DATALINK_PROT_A5_CTRL, NETWORK_TYP_CTRL_HEAT)        \
  XX(13, CTRL_HEAT_SET,    network_msg_ctrl_heat_set_t,    DATALINK_PROT_A5_CTRL, NETWORK_TYP_CTRL_HEAT_SET)    \
  XX(14, CTRL_LAYOUT_REQ,  network_msg_ctrl_layout_req,    DATALINK_PROT_A5_CTRL, NETWORK_TYP_CTRL_LAYOUT_REQ)  \
  XX(15, CTRL_LAYOUT,      network_msg_ctrl_layout_t,      DATALINK_PROT_A5_CTRL, NETWORK_TYP_CTRL_LAYOUT)      \
  XX(16, CTRL_LAYOUT_SET,  network_msg_ctrl_layout_set_t,  DATALINK_PROT_A5_CTRL, NETWORK_TYP_CTRL_LAYOUT_SET)  \
  XX(17, PUMP_REG_SET,     network_msg_pump_reg_set_t,     DATALINK_PROT_A5_PUMP, NETWORK_TYP_PUMP_REG)         \
  XX(18, PUMP_REG_RESP,    network_msg_pump_reg_resp_t,    DATALINK_PROT_A5_PUMP, NETWORK_TYP_PUMP_REG)         \
  XX(19, PUMP_CTRL_SET,    network_msg_pump_ctrl_t,        DATALINK_PROT_A5_PUMP, NETWORK_TYP_PUMP_CTRL)        \
  XX(20, PUMP_CTRL_RESP,   network_msg_pump_ctrl_t,        DATALINK_PROT_A5_PUMP, NETWORK_TYP_PUMP_CTRL)        \
  XX(21, PUMP_MODE_SET,    network_msg_pump_mode_t,        DATALINK_PROT_A5_PUMP, NETWORK_TYP_PUMP_MODE)        \
  XX(22, PUMP_MODE_RESP,   network_msg_pump_mode_t,        DATALINK_PROT_A5_PUMP, NETWORK_TYP_PUMP_MODE)        \
  XX(23, PUMP_RUN_SET,     network_msg_pump_run_t,         DATALINK_PROT_A5_PUMP, NETWORK_TYP_PUMP_RUN)         \
  XX(24, PUMP_RUN_RESP,    network_msg_pump_run_t,         DATALINK_PROT_A5_PUMP, NETWORK_TYP_PUMP_RUN)         \
  XX(25, PUMP_STATUS_REQ,  network_msg_pump_status_req_t,  DATALINK_PROT_A5_PUMP, NETWORK_TYP_PUMP_STATUS)      \
  XX(26, PUMP_STATUS_RESP, network_msg_pump_status_resp_t, DATALINK_PROT_A5_PUMP, NETWORK_TYP_PUMP_STATUS)      \
  XX(27, CHLOR_PING_REQ,   network_msg_chlor_ping_req_t,   DATALINK_PROT_IC,      NETWORK_TYP_CHLOR_PING_REQ)   \
  XX(28, CHLOR_PING_RESP,  network_msg_chlor_ping_resp_t,  DATALINK_PROT_IC,      NETWORK_TYP_CHLOR_PING_RESP)  \
  XX(29, CHLOR_NAME,       network_msg_chlor_name_t,       DATALINK_PROT_IC,      NETWORK_TYP_CHLOR_NAME)       \
  XX(30, CHLOR_LEVEL_SET,  network_msg_chlor_level_set_t,  DATALINK_PROT_IC,      NETWORK_TYP_CHLOR_LEVEL_SET)  \
  XX(31, CHLOR_LEVEL_RESP, network_msg_chlor_level_resp_t, DATALINK_PROT_IC,      NETWORK_TYP_CHLOR_LEVEL_RESP)

typedef enum {
#define XX(num, name, typ, proto, proto_typ) MSG_TYP_##name = num,
  MSG_TYP_MAP(XX)
#undef XX
} msg_typ_t;
