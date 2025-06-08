#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// RCC peripheral simulation
typedef struct {
    volatile uint32_t CR;
    volatile uint32_t PLLCFGR;
    volatile uint32_t CFGR;
    volatile uint32_t CIR;
    volatile uint32_t AHB1ENR;
    volatile uint32_t AHB2ENR;
    volatile uint32_t APB1ENR;
    volatile uint32_t APB2ENR;
} RCC_TypeDef;

extern RCC_TypeDef Sim_RCC;
#define RCC (&Sim_RCC)

// GPIO peripheral simulation (only what you touch)
typedef struct {
    volatile uint32_t MODER;
    volatile uint32_t OTYPER;
    volatile uint32_t OSPEEDR;
    volatile uint32_t PUPDR;
    volatile uint32_t IDR;
    volatile uint32_t ODR;
    volatile uint32_t BSRR;
    volatile uint32_t LCKR;
    volatile uint32_t AFR[2];
} GPIO_TypeDef;

extern GPIO_TypeDef Sim_GPIOA;
#define GPIOA (&Sim_GPIOA)

// USART peripheral simulation
typedef struct {
    volatile uint32_t SR;
    volatile uint32_t DR;
    volatile uint32_t BRR;
    volatile uint32_t CR1;
    volatile uint32_t CR2;
    volatile uint32_t CR3;
    volatile uint32_t GTPR;
} USART_TypeDef;

extern USART_TypeDef Sim_USART2;
#define USART2 (&Sim_USART2)

// USART flags (copy from stm32f4xx.h)
#define USART_SR_RXNE (1U << 5)
#define USART_SR_TXE  (1U << 7)
#define USART_CR1_UE  (1U << 13)
#define USART_CR1_TE  (1U << 3)
#define USART_CR1_RE  (1U << 2)
#define USART_CR1_RXNEIE (1U << 5)
#define USART_CR1_TXEIE  (1U << 7)
#define RCC_AHB1ENR_GPIOAEN (1U << 0)
#define RCC_APB1ENR_USART2EN (1U << 17)

#ifdef __cplusplus
}
#endif
