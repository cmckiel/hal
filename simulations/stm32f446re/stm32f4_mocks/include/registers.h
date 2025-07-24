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

extern GPIO_TypeDef Sim_GPIOB;
#define GPIOB (&Sim_GPIOB)

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

extern USART_TypeDef Sim_USART1;
#define USART1 (&Sim_USART1)

extern USART_TypeDef Sim_USART2;
#define USART2 (&Sim_USART2)

/**
  * @brief Inter-integrated Circuit Interface
  */
typedef struct
{
  volatile uint32_t CR1;        /*!< I2C Control register 1,     Address offset: 0x00 */
  volatile uint32_t CR2;        /*!< I2C Control register 2,     Address offset: 0x04 */
  volatile uint32_t OAR1;       /*!< I2C Own address register 1, Address offset: 0x08 */
  volatile uint32_t OAR2;       /*!< I2C Own address register 2, Address offset: 0x0C */
  volatile uint32_t DR;         /*!< I2C Data register,          Address offset: 0x10 */
  volatile uint32_t SR1;        /*!< I2C Status register 1,      Address offset: 0x14 */
  volatile uint32_t SR2;        /*!< I2C Status register 2,      Address offset: 0x18 */
  volatile uint32_t CCR;        /*!< I2C Clock control register, Address offset: 0x1C */
  volatile uint32_t TRISE;      /*!< I2C TRISE register,         Address offset: 0x20 */
  volatile uint32_t FLTR;       /*!< I2C FLTR register,          Address offset: 0x24 */
} I2C_TypeDef;

extern I2C_TypeDef Sim_I2C1;
#define I2C1 (&Sim_I2C1)

typedef struct
{
  volatile uint32_t CTRL;                   /*!< Offset: 0x000 (R/W)  SysTick Control and Status Register */
  volatile uint32_t LOAD;                   /*!< Offset: 0x004 (R/W)  SysTick Reload Value Register */
  volatile uint32_t VAL;                    /*!< Offset: 0x008 (R/W)  SysTick Current Value Register */
  volatile uint32_t CALIB;                  /*!< Offset: 0x00C (R/ )  SysTick Calibration Register */
} SysTick_Type;

extern SysTick_Type Sim_SysTick;
#define SysTick (&Sim_SysTick)

// USART flags (copy from stm32f4xx.h)
#define USART_SR_RXNE (1U << 5)
#define USART_SR_TXE  (1U << 7)
#define USART_CR1_M   (1U << 12)
#define USART_CR1_UE  (1U << 13)
#define USART_CR1_TE  (1U << 3)
#define USART_CR1_RE  (1U << 2)
#define USART_CR1_RXNEIE (1U << 5)
#define USART_CR1_TXEIE  (1U << 7)

// RCC defines
#define RCC_AHB1ENR_GPIOAEN (1U << 0)
#define RCC_AHB1ENR_GPIOBEN (1U << 1)
#define RCC_APB1ENR_USART2EN (1U << 17)
#define RCC_APB2ENR_USART1EN (1U << 4)
#define RCC_APB1ENR_I2C1EN (1U << 21)

// I2C defines
#define I2C_CR2_FREQ (0x3F) // first 6 bits of CR2
#define I2C_TRISE_TRISE (0x3F) // first 6 bits of TRISE
#define I2C_CCR_CCR (0xFFF) // first 12 bits
#define I2C_CR1_PE (1U << 0)
#define I2C_CCR_FS (1U << 15)
#define GPIO_OTYPER_OT_8 (1U << 8)
#define GPIO_OTYPER_OT_9 (1U << 9)

#ifdef __cplusplus
}
#endif
