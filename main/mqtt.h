#ifndef MQTT_H
#define MQTT_H

extern uint32_t mqtt_publish(const char *topic, const char *data, uint32_t len, uint8_t qos, uint8_t retain);
extern void mqtt_app_start(void);
extern uint8_t mqtt_isConnected(void);


#endif