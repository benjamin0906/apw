#ifndef ADC_H_
#define ADC_H_

extern void adc_init(void);
extern uint16_t adc_get_volt(void);
extern uint16_t adc_get_raw(void);

#endif