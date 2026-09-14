#ifndef MQTT_H
#define MQTT_H

#include "stdint.h"

typedef enum eMqttState
{
    MQTT_State_Uninitialized,
    MQTT_State_Disconnected,
    MQTT_State_Connecting,
    MQTT_State_Connected,
    MQTT_State_Disconnecting,
} dtMQTT_State;

extern uint32_t mqtt_publish(const char *topic, const char *data, uint32_t len, uint8_t qos, uint8_t retain);
extern void mqtt_init(const uint8_t *const broker_url);
extern void mqtt_app_start(void);
extern uint8_t mqtt_isConnected(void);
extern void mqtt_subscribe(uint8_t *topic);
extern uint8_t mqtt_get_data(uint8_t *topic, uint8_t *data);
extern void mqtt_reconnect(void);
extern void mqtt_stop(void);
extern dtMQTT_State MQTT_GetState(void);

#endif