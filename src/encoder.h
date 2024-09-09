#pragma once

#include <stdbool.h>
#include <ch32v00x.h>

#ifdef USE_SPL
#define ENC_RCC     RCC_APB2Periph_GPIOA
#else
#define ENC_RCC     RCC_IOPAEN
#endif
#define ENC_GPIO    GPIOA
#define ENC_CLK     1
#define ENC_DT      2

#ifdef USE_SPL
#define BTN_RCC     RCC_APB2Periph_GPIOD
#else
#define BTN_RCC     RCC_IOPDEN
#endif
#define BTN_GPIO    GPIOD
#define BTN_PIN     4

void Encoder_Init();
int8_t Encoder_Read();
bool Encoder_Button();
