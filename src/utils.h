#pragma once

#include <inttypes.h>

void UPDATE_REG16(volatile uint16_t *addr, uint16_t and_mask, uint16_t or_mask);
void UPDATE_REG32(volatile uint32_t *addr, uint32_t and_mask, uint32_t or_mask);

void delay_ms(uint32_t ms);
