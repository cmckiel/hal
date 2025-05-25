#ifndef _ADC_H
#define _ADC_H

#include <stdint.h>

void pa1_adc_init(void);
void start_conversion(void);
uint32_t adc_read(void);

#endif /* _ADC_H */