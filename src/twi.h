#pragma once

#include <stdint.h>
#include <stdbool.h>

#define TWI_FLAG_AUTOSTOP   1
#define TWI_FLAG_WAIT       2

typedef enum { TWI_OK, TWI_BUSY, TWI_TIMEOUT, TWI_NACK } twi_err;

void TWI_Init(uint32_t speed);
twi_err TWI_Start(uint8_t addr, bool receiver);
void TWI_Stop();
void TWI_Read(uint8_t *buf, uint16_t size);
void TWI_Write(const uint8_t *buf, uint16_t size, uint8_t flags);
