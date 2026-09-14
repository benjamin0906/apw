#ifndef _WIFI_H_
#define _WIFI_H_

extern void WIFI_Init(void);
extern uint8_t WIFI_IsConnected(void);
extern void WIFI_Stop(void);
extern void WIFI_Start(void);

#endif /* _WIFI_H_ */