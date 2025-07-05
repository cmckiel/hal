#ifdef __cplusplus
extern "C" {
#endif

#include "stdlib.h"
#include "stdbool.h"

#define USART2_IRQn 38
#define USART1_IRQn 37

void NVIC_EnableIRQ(size_t interrupt_number);
bool NVIC_IsIRQEnabled(size_t interrupt_number);
void NVIC_DisableIRQ(size_t interrupt_number);

#ifdef __cplusplus
}
#endif
