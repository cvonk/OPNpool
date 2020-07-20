#pragma once

typedef struct {
    QueueHandle_t toDisplayQ;
    QueueHandle_t toMqttQ;
} display_task_ipc_t;

void display_task(void * ipc_void);

