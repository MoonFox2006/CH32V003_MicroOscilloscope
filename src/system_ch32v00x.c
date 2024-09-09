/********************************** (C) COPYRIGHT *******************************
 * File Name          : system_ch32v00x.c
 * Author             : WCH
 * Version            : V1.0.0
 * Date               : 2023/12/26
 * Description        : CH32V00x Device Peripheral Access Layer System Source File.
*********************************************************************************
* Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
* Attention: This software (modified or not) and binary are used for 
* microcontroller manufactured by Nanjing Qinheng Microelectronics.
*******************************************************************************/
#include <ch32v00x.h>
#include <stdio.h>
#include "utils.h"

/*********************************************************************
 * @fn      SystemInit
 *
 * @brief   Setup the microcontroller system Initialize the Embedded Flash Interface,
 *        the PLL and update the SystemCoreClock variable.
 *
 * @return  none
 */
void SystemInit(void) {
    RCC->CTLR |= 0x00000001;
    RCC->CFGR0 &= 0xF8FF0000;
    RCC->CTLR &= 0xFEF6FFFF;
    RCC->CTLR &= 0xFFFBFFFF;
    RCC->CFGR0 &= 0xFFFEFFFF;
    RCC->INTR = 0x009F0000;

    if (*(uint8_t*)CFG0_PLL_TRIM != 0xFF) {
        UPDATE_REG32(&RCC->CTLR, 0xFFFFFF07, (*(uint8_t*)CFG0_PLL_TRIM & 0x1F) << 3); // CTLR_HSITRIM_Mask
    } else {
        UPDATE_REG32(&RCC->CTLR, 0xFFFFFF07, 0x10 << 3); // CTLR_HSITRIM_Mask
    }

    /* Flash 1 wait state */
    FLASH->ACTLR &= ~(uint32_t)FLASH_ACTLR_LATENCY;
    FLASH->ACTLR |= (uint32_t)FLASH_ACTLR_LATENCY_1;

    /* HCLK = SYSCLK = APB1 */
//    RCC->CFGR0 |= (uint32_t)RCC_HPRE_DIV1;

    /* PLL configuration: PLLCLK = HSI * 2 = 48 MHz */
    RCC->CFGR0 &= ~RCC_PLLSRC;
    RCC->CFGR0 |= RCC_PLLSRC_HSI_Mul2;

    /* Enable PLL */
    RCC->CTLR |= RCC_PLLON;
    /* Wait till PLL is ready */
    while ((RCC->CTLR & RCC_PLLRDY) == 0) {}
    /* Select PLL as system clock source */
    RCC->CFGR0 &= ~RCC_SW;
    RCC->CFGR0 |= RCC_SW_PLL;    
    /* Wait till PLL is used as system clock source */
    while ((RCC->CFGR0 & RCC_SWS) != (uint32_t)0x08) {}
}

/*********************************************************************
 * @fn      _sbrk
 *
 * @brief   Change the spatial position of data segment.
 *
 * @return  size: Data length
 */
void* __attribute__((used)) _sbrk(ptrdiff_t incr) {
    extern char _end[];
    extern char _heap_end[];
    static char *curbrk = _end;

    if ((curbrk + incr < _end) || (curbrk + incr > _heap_end))
        return NULL - 1;

    curbrk += incr;
    return curbrk - incr;
}

/*********************************************************************
 * @fn      NMI_Handler
 *
 * @brief   This function handles NMI exception.
 *
 * @return  none
 */
void __attribute__((interrupt("WCH-Interrupt-fast"))) NMI_Handler(void) {
    while (1) {}
}

/*********************************************************************
 * @fn      HardFault_Handler
 *
 * @brief   This function handles Hard Fault exception.
 *
 * @return  none
 */
void __attribute__((interrupt("WCH-Interrupt-fast"))) HardFault_Handler(void) {
    while (1) {}
}
