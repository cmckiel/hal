#include "nvic.h"

static bool uart1_isr_enabled = false;
static bool uart2_isr_enabled = false;

void NVIC_EnableIRQ(size_t interrupt_number)
{
    if (interrupt_number == USART1_IRQn)
    {
        uart1_isr_enabled = true;
    }
    else if (interrupt_number == USART2_IRQn)
    {
        uart2_isr_enabled = true;
    }
}

bool NVIC_IsIRQEnabled(size_t interrupt_number)
{
    bool res = false;

    if (interrupt_number == USART1_IRQn)
    {
        res = uart1_isr_enabled;
    }
    else if (interrupt_number == USART2_IRQn)
    {
        res = uart2_isr_enabled;
    }

    return res;
}

void NVIC_DisableIRQ(size_t interrupt_number)
{
    if (interrupt_number == USART1_IRQn)
    {
        uart1_isr_enabled = false;
    }
    else if (interrupt_number == USART2_IRQn)
    {
        uart2_isr_enabled = false;
    }
}
