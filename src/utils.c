#include <ch32v00x.h>
#include "utils.h"

void UPDATE_REG16(volatile uint16_t *addr, uint16_t and_mask, uint16_t or_mask) {
    and_mask &= *addr;
    and_mask |= or_mask;
    *addr = and_mask;
}

void UPDATE_REG32(volatile uint32_t *addr, uint32_t and_mask, uint32_t or_mask) {
    and_mask &= *addr;
    and_mask |= or_mask;
    *addr = and_mask;
}

void delay_ms(uint32_t ms) {
    SysTick->SR = 0;
    SysTick->CMP = (FCPU / 8000) * ms;
    SysTick->CNT = 0;
    SysTick->CTLR = 1 << 0;

    while (! (SysTick->SR & 0x01)) {}
    SysTick->CTLR = 0;
}
