#ifdef USE_SPL
#include <ch32v00x_rcc.h>
#include <ch32v00x_gpio.h>
#include <ch32v00x_exti.h>
#else
#include "utils.h"
#endif
#include "encoder.h"

static volatile int8_t _enc_dir = 0;

void Encoder_Init() {
#ifdef USE_SPL
    GPIO_InitTypeDef GPIO_InitStructure = { 0 };
    EXTI_InitTypeDef EXTI_InitStructure = { 0 };
    NVIC_InitTypeDef NVIC_InitStructure = { 0 };

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO | ENC_RCC | BTN_RCC, ENABLE);

    GPIO_InitStructure.GPIO_Pin = (1 << ENC_CLK) | (1 << ENC_DT);
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_Init(ENC_GPIO, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin = 1 << BTN_PIN;
    GPIO_Init(BTN_GPIO, &GPIO_InitStructure);

    uint8_t GPIO_PortSource = (ENC_GPIO == GPIOA ? GPIO_PortSourceGPIOA : (ENC_GPIO == GPIOC ? GPIO_PortSourceGPIOC : GPIO_PortSourceGPIOD));

    GPIO_EXTILineConfig(GPIO_PortSource, ENC_CLK);
    GPIO_EXTILineConfig(GPIO_PortSource, ENC_DT);
    EXTI_InitStructure.EXTI_Line = (1 << ENC_CLK) | (1 << ENC_DT);
    EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
    EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising_Falling;
    EXTI_InitStructure.EXTI_LineCmd = ENABLE;
    EXTI_Init(&EXTI_InitStructure);

    NVIC_InitStructure.NVIC_IRQChannel = EXTI7_0_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
#else
    RCC->APB2PCENR |= (RCC_AFIOEN | ENC_RCC | BTN_RCC);

    UPDATE_REG32(&ENC_GPIO->CFGLR, ~((0x0F << (ENC_CLK * 4)) | (0x0F << (ENC_DT * 4))),
        (0x08 << (ENC_CLK * 4)) | (0x08 << (ENC_DT * 4)));
    ENC_GPIO->BSHR = (1 << ENC_CLK) | (1 << ENC_DT);
    UPDATE_REG32(&BTN_GPIO->CFGLR, ~(0x0F << (BTN_PIN * 4)), 0x08 << (BTN_PIN * 4));
    BTN_GPIO->BSHR = 1 << BTN_PIN;

    uint8_t GPIO_PortSource = (ENC_GPIO == GPIOA ? 0 : (ENC_GPIO == GPIOC ? 2 : 3));

    UPDATE_REG32(&AFIO->EXTICR, ~((0x03 << (ENC_CLK * 2)) | (0x03 << (ENC_DT * 2))),
        (GPIO_PortSource << (ENC_CLK * 2)) | (GPIO_PortSource << (ENC_DT * 2)));

    EXTI->RTENR |= (1 << ENC_CLK) | (1 << ENC_DT);
    EXTI->FTENR |= (1 << ENC_CLK) | (1 << ENC_DT);
    EXTI->INTENR |= (1 << ENC_CLK) | (1 << ENC_DT);

    NVIC_SetPriority(EXTI7_0_IRQn, (0 << 7) | (1 << 6));
    NVIC_EnableIRQ(EXTI7_0_IRQn);
#endif
    _enc_dir = 0;
}

int8_t Encoder_Read() {
    int8_t result = _enc_dir;

    _enc_dir = 0;
    return result;
}

#ifdef USE_SPL
bool Encoder_Button() {
    return GPIO_ReadInputDataBit(BTN_GPIO, 1 << BTN_PIN) == Bit_RESET;
}
#else
inline __attribute__((always_inline)) bool Encoder_Button() {
    return (BTN_GPIO->INDR & (1 << BTN_PIN)) == 0;
}
#endif

void __attribute__((interrupt("WCH-Interrupt-fast"))) EXTI7_0_IRQHandler(void) {
    const int8_t ENC_STATES[] = {
        0, 1, -1, 0,
        -1, 0, 0, 1,
        1, 0, 0, -1,
        0, -1, 1, 0
    };

    static uint8_t ab = 0x03;
    static int8_t e = 0;

#ifdef USE_SPL
    uint8_t indr = GPIO_ReadInputData(ENC_GPIO);
#else
    uint8_t indr = ENC_GPIO->INDR;
#endif

    ab <<= 2;
#if ENC_DT == ENC_CLK + 1
    ab |= ((indr >> ENC_CLK) & 0x03);
#else
    ab |= (((indr >> ENC_DT) & 0x01) << 1);
    ab |= ((indr >> ENC_CLK) & 0x01);
#endif

    e += ENC_STATES[ab & 0x0F];
    if (e < -3) {
        _enc_dir = -1;
        e = 0;
    } else if (e > 3) {
        _enc_dir = 1;
        e = 0;
    }
#ifdef USE_SPL
    EXTI_ClearITPendingBit((1 << ENC_CLK) | (1 << ENC_DT));
#else
    EXTI->INTFR = (1 << ENC_CLK) | (1 << ENC_DT);
#endif
}
