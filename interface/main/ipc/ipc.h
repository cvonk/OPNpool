#pragma once
#include <esp_types.h>

#include "../tx_buf/tx_buf.h"

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(a) (sizeof(a) / sizeof(*(a)))
#endif
#define ALIGN( type ) __attribute__((aligned( __alignof__( type ) )))
#define PACK( type )  __attribute__((aligned( __alignof__( type ) ), packed ))
#define PACK8  __attribute__((aligned( __alignof__( uint8_t ) ), packed ))
#ifndef MIN
#define MIN(a,b) ({ __typeof__ (a) _a = (a); __typeof__ (b) _b = (b); _a < _b ? _a : _b; })
#endif
#ifndef MIN
#define MAX(a,b) ({ __typeof__ (a) _a = (a); __typeof__ (b) _b = (b); _a > _b ? _a : _b; })
#endif

#define WIFI_DEVNAME_LEN (32)
#define WIFI_DEVIPADDR_LEN (16)

typedef struct ipc_t {
    QueueHandle_t to_mqtt_q;
    struct dev {
        char ipAddr[WIFI_DEVIPADDR_LEN];
        char name[WIFI_DEVNAME_LEN];
        struct count {
            uint wifiAuthErr;
            uint wifiConnect;
            uint mqttConnect;
        } count;
    } dev;
} ipc_t;

#define IPC_TO_MQTT_TYP_MAP(XX) \
  XX(0x00, STATE)   \
  XX(0x01, RESTART) \
  XX(0x02, WHO)     \
  XX(0x03, DBG)

typedef enum {
#define XX(num, name) IPC_TO_MQTT_TYP_##name = num,
  IPC_TO_MQTT_TYP_MAP(XX)
#undef XX
} ipc_to_mqtt_typ_t;

typedef struct toMqttMsg_t {
    ipc_to_mqtt_typ_t dataType;
    char *            data;  // must be freed by recipient
} toMqttMsg_t;

void ipc_send_to_mqtt(ipc_to_mqtt_typ_t const dataType, char const * const data, ipc_t const * const ipc);
char const * ipc_to_mqtt_typ_str(ipc_to_mqtt_typ_t const typ);
