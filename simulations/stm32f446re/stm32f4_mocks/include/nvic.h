#ifdef __cplusplus
extern "C" {
#endif

// Very minimal NVIC simulation
#define NVIC_EnableIRQ(x) ((void)(x))  // No-op

#ifdef __cplusplus
}
#endif
