#pragma once
#include <esp_system.h>

#include "../datalink/datalink.h"
#include "../datalink/datalink_pkt.h"

#include "network_msg.h"

/* network.c */
uint8_t network_ic_len(uint8_t const ic_typ);

/* network_rx.c */
bool network_rx_msg(datalink_pkt_t const * const pkt, network_msg_t * const msg, bool * const txOpportunity);

/* network_tx.c */
bool network_tx_msg(network_msg_t const * const msg, datalink_pkt_t * const pkt);

/* network_str.c */
char const * network_date_str(uint8_t const year, uint8_t const month, uint8_t const day);
char const * network_time_str(uint8_t const hours, uint8_t const minutes);
char const * network_version_str(uint8_t const major, uint8_t const minor);
const char * network_circuit_str(network_circuit_t const circuit);
const char * network_pump_mode_str(network_pump_mode_t const pump_mode);
char const * network_pump_prg_str(uint16_t const address);
const char * network_heat_src_str(network_heat_src_t const heat_src);
char const * network_typ_pump_str(network_typ_pump_t typ);
char const * network_typ_ctrl_str(network_typ_ctrl_t typ);
char const * network_typ_chlor_str(network_typ_chlor_t typ);
const char * network_msg_typ_str(network_msg_typ_t const typ);
int network_heat_src_nr(char const * const heat_src_str);
int network_circuit_nr(char const * const circuit_str);
int network_msg_typ_nr(char const * const msg_typ_str);
