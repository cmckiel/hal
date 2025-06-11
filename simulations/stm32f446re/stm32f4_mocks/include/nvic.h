#ifdef __cplusplus
extern "C" {
#endif

#define USART2_IRQn 38
#define USART1_IRQn 37

// Very minimal NVIC simulation
#define NVIC_EnableIRQ(x) ((void)(x))  // No-op

#ifdef __cplusplus
}
#endif
