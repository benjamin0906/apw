#ifndef _WATERING_CONTROL_H_
#define _WATERING_CONTROL_H_

#include "stdint.h"

extern void WC_Runnable(void);
extern void WC_CmdCallback(uint8_t *cmd);
extern uint8_t WC_StateCallback(uint8_t *state);
extern uint8_t WC_StateCb(uint8_t *state);

#endif /* _WATERING_CONTROL_H_ */