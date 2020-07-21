#pragma once
#include <esp_types.h>

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
    QueueHandle_t toMqttQ;
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

typedef enum toMqttMsgType_t {
    TO_MQTT_MSGTYPE_RESTART,
    TO_MQTT_MSGTYPE_WHO,
    TO_MQTT_MSGTYPE_DBG,
} toMqttMsgType_t;

typedef struct toMqttMsg_t {
    toMqttMsgType_t dataType;
    char * data;  // must be freed by recipient
} toMqttMsg_t;

void sendToMqtt(toMqttMsgType_t const dataType, char const * const data, ipc_t const * const ipc);
