#include <ch32v00x.h>
#ifdef USE_SPL
#include <ch32v00x_rcc.h>
#include <ch32v00x_gpio.h>
#include <ch32v00x_i2c.h>
#include <ch32v00x_dma.h>
#else
#include "utils.h"
#endif
#include "twi.h"

#ifndef USE_SPL
#define I2C_MASTER_MODE_SELECT                  0x00030001 /* BUSY, MSL and SB flag */
#define I2C_MASTER_TRANSMITTER_MODE_SELECTED    0x00070082 /* BUSY, MSL, ADDR, TXE and TRA flags */
#define I2C_MASTER_RECEIVER_MODE_SELECTED       0x00030002 /* BUSY, MSL and ADDR flags */
#define I2C_MASTER_BYTE_TRANSMITTED             0x00070084 /* TRA, BUSY, MSL, TXE and BTF flags */
#endif

#define TWI_WAIT    32768

static volatile uint8_t _twi_flags = 0;

void TWI_Init(uint32_t speed) {
#ifdef USE_SPL
    GPIO_InitTypeDef GPIO_InitStructure = { 0 };
    I2C_InitTypeDef I2C_InitTSturcture = { 0 };
    DMA_InitTypeDef DMA_InitStructure = { 0 };
    NVIC_InitTypeDef NVIC_InitStructure = { 0 };

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_I2C1, ENABLE);
    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1 | GPIO_Pin_2;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_OD;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_30MHz;
    GPIO_Init(GPIOC, &GPIO_InitStructure);

    I2C_InitTSturcture.I2C_ClockSpeed = speed;
    I2C_InitTSturcture.I2C_Mode = I2C_Mode_I2C;
    I2C_InitTSturcture.I2C_DutyCycle = speed > 100000 ? I2C_DutyCycle_16_9 : I2C_DutyCycle_2;
    I2C_InitTSturcture.I2C_OwnAddress1 = 0;
    I2C_InitTSturcture.I2C_Ack = I2C_Ack_Enable;
    I2C_InitTSturcture.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit;
    I2C_Init(I2C1, &I2C_InitTSturcture);

    I2C_DMACmd(I2C1, ENABLE);
    I2C_DMALastTransferCmd(I2C1, ENABLE);

    DMA_DeInit(DMA1_Channel6);

    DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)&I2C1->DATAR;
    DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralDST;
    DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
    DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
    DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
    DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
    DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;
    DMA_InitStructure.DMA_Priority = DMA_Priority_VeryHigh;
    DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;
    DMA_Init(DMA1_Channel6, &DMA_InitStructure);

//    DMA_Cmd(DMA1_Channel6, ENABLE);

    DMA_DeInit(DMA1_Channel7);

//    DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)&I2C1->DATAR;
    DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralSRC;
//    DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
//    DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
//    DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
//    DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
//    DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;
//    DMA_InitStructure.DMA_Priority = DMA_Priority_VeryHigh;
//    DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;
    DMA_Init(DMA1_Channel7, &DMA_InitStructure);

//    DMA_Cmd(DMA1_Channel7, ENABLE);

    NVIC_InitStructure.NVIC_IRQChannel = DMA1_Channel6_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

    NVIC_InitStructure.NVIC_IRQChannel = DMA1_Channel7_IRQn;
    NVIC_Init(&NVIC_InitStructure);

    DMA_ITConfig(DMA1_Channel6, DMA_IT_TC, ENABLE);
    DMA_ITConfig(DMA1_Channel7, DMA_IT_TC, ENABLE);

    I2C_Cmd(I2C1, ENABLE);
#else
    RCC->APB2PCENR |= RCC_IOPCEN;
    RCC->APB1PCENR |= RCC_I2C1EN;
    RCC->AHBPCENR |= RCC_DMA1EN;

    UPDATE_REG32(&GPIOC->CFGLR, ~((0x0F << 4) | (0x0F << 8)), (0x0D << 4) | (0x0D << 8));

    uint16_t ckcfgr;

    I2C1->CTLR1 &= ~I2C_CTLR1_PE;
    UPDATE_REG16(&I2C1->CTLR2, ~I2C_CTLR2_FREQ, (uint32_t)FCPU / 1000000);
    if (speed <= 100000) {
        ckcfgr = (uint32_t)FCPU / (speed << 1);
        if (ckcfgr < 0x04)
            ckcfgr = 0x04;
    } else {
        ckcfgr = (uint32_t)FCPU / (speed * 25);
        ckcfgr |= I2C_CKCFGR_DUTY;
        if ((ckcfgr & I2C_CKCFGR_CCR) == 0) {
            ckcfgr |= 0x0001;
        }
        ckcfgr |= I2C_CKCFGR_FS;
    }
    I2C1->CKCFGR = ckcfgr;
    UPDATE_REG16(&I2C1->CTLR1, 0xFBF5, I2C_CTLR1_ACK | I2C_CTLR1_PE); // CTLR1_CLEAR_Mask

    I2C1->CTLR2 |= (I2C_CTLR2_DMAEN | I2C_CTLR2_LAST);

    DMA1_Channel6->CFGR = DMA_CFGR1_DIR | DMA_CFGR1_MINC | DMA_CFGR1_PL_0 | DMA_CFGR1_PL_1 | DMA_CFGR1_TCIE;
    DMA1_Channel6->CNTR = 0;
    DMA1_Channel6->PADDR = (uint32_t)&I2C1->DATAR;
    DMA1_Channel6->MADDR = 0;
    DMA1_Channel7->CFGR = DMA_CFGR1_MINC | DMA_CFGR1_PL_0 | DMA_CFGR1_PL_1 | DMA_CFGR1_TCIE;
    DMA1_Channel7->CNTR = 0;
    DMA1_Channel7->PADDR = (uint32_t)&I2C1->DATAR;
    DMA1_Channel7->MADDR = 0;
    DMA1->INTFCR |= (DMA_GIF6 | DMA_TCIF6 | DMA_HTIF6 | DMA_TEIF6 | DMA_GIF7 | DMA_TCIF7 | DMA_HTIF7 | DMA_TEIF7);

    NVIC_SetPriority(DMA1_Channel6_IRQn, (0 << 7) | (1 << 6));
    NVIC_EnableIRQ(DMA1_Channel6_IRQn);
    NVIC_SetPriority(DMA1_Channel7_IRQn, (0 << 7) | (1 << 6));
    NVIC_EnableIRQ(DMA1_Channel7_IRQn);

//    DMA1_Channel6->CFGR |= DMA_CFGR1_TCIE;
//    DMA1_Channel7->CFGR |= DMA_CFGR1_TCIE;

//    I2C1->CTLR1 |= I2C_CTLR1_PE;
#endif
}

#ifndef USE_SPL
static uint32_t TWI_State() {
    uint32_t result;

    result = I2C1->STAR1;
    result |= (I2C1->STAR2 << 16);
    return result;
}

static bool TWI_Wait(uint32_t states) {
    volatile uint32_t timeout;

    timeout = TWI_WAIT;
    while ((TWI_State() & states) != states) {
        if (! timeout--)
            return false;
    }

    return true;
}
#endif

twi_err TWI_Start(uint8_t addr, bool receiver) {
    volatile uint32_t timeout;

#ifdef USE_SPL
    timeout = TWI_WAIT;
    while (I2C_GetFlagStatus(I2C1, I2C_FLAG_BUSY) != RESET) {
        if (! timeout--)
            return TWI_BUSY;
    }
    I2C_GenerateSTART(I2C1, ENABLE);

    timeout = TWI_WAIT;
    while (! I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_MODE_SELECT)) {
        if (! timeout--) {
            I2C_GenerateSTART(I2C1, DISABLE);
            return TWI_TIMEOUT;
        }
    }
    I2C_Send7bitAddress(I2C1, addr << 1, receiver ? I2C_Direction_Receiver : I2C_Direction_Transmitter);

    timeout = TWI_WAIT;
    while (! I2C_CheckEvent(I2C1, receiver ? I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED : I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED)) {
        if (! timeout--) {
            I2C_GenerateSTART(I2C1, DISABLE);
            return TWI_NACK;
        }
    }
#else
    timeout = TWI_WAIT;
    while (I2C1->STAR2 & I2C_STAR2_BUSY) {
        if (! timeout--)
            return TWI_BUSY;
    }
    I2C1->CTLR1 |= (I2C_CTLR1_START | I2C_CTLR1_ACK);

    if (! TWI_Wait(I2C_MASTER_MODE_SELECT)) {
        I2C1->CTLR1 &= ~I2C_CTLR1_START;
        return TWI_TIMEOUT;
    }

    I2C1->DATAR = (addr << 1) | receiver;

    if (! TWI_Wait(receiver ? I2C_MASTER_RECEIVER_MODE_SELECTED : I2C_MASTER_TRANSMITTER_MODE_SELECTED)) {
        I2C1->CTLR1 &= ~I2C_CTLR1_START;
        return TWI_NACK;
    }
#endif

    return TWI_OK;
}

inline void __attribute__((always_inline)) TWI_Stop() {
#ifdef USE_SPL
    I2C_GenerateSTOP(I2C1, ENABLE);
#else
    I2C1->CTLR1 |= I2C_CTLR1_STOP;
#endif
}

void TWI_Read(uint8_t *buf, uint16_t size) {
#ifdef USE_SPL
    DMA_SetCurrDataCounter(DMA1_Channel7, size);
#else
    DMA1_Channel7->CNTR = size;
#endif
    DMA1_Channel7->MADDR = (uint32_t)buf;
    _twi_flags = TWI_FLAG_AUTOSTOP;
#ifdef USE_SPL
    DMA_Cmd(DMA1_Channel7, ENABLE);
#else
    DMA1_Channel7->CFGR |= DMA_CFGR1_EN;
#endif
    while (_twi_flags & TWI_FLAG_WAIT) {}
}

void TWI_Write(const uint8_t *buf, uint16_t size, uint8_t flags) {
#ifdef USE_SPL
    DMA_SetCurrDataCounter(DMA1_Channel6, size);
#else
    DMA1_Channel6->CNTR = size;
#endif
    DMA1_Channel6->MADDR = (uint32_t)buf;
    _twi_flags = flags;
#ifdef USE_SPL
    DMA_Cmd(DMA1_Channel6, ENABLE);
#else
    DMA1_Channel6->CFGR |= DMA_CFGR1_EN;
#endif
    while (_twi_flags & TWI_FLAG_WAIT) {}
}

void __attribute__((interrupt("WCH-Interrupt-fast"))) DMA1_Channel6_IRQHandler() {
#ifdef USE_SPL
    if (DMA_GetITStatus(DMA1_IT_TC6)) {
        DMA_Cmd(DMA1_Channel6, DISABLE);
        DMA_ClearITPendingBit(DMA1_IT_TC6);
        if (_twi_flags & TWI_FLAG_AUTOSTOP) {
            volatile uint32_t timeout;

            timeout = TWI_WAIT;
            while (! I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_TRANSMITTED)) {
                if (! timeout--)
                    break;
            }
            TWI_Stop();
        }
        _twi_flags = 0;
    }
#else
    if (DMA1->INTFR & DMA_TCIF6) {
        DMA1_Channel6->CFGR &= ~(uint32_t)DMA_CFGR1_EN;
        DMA1->INTFCR = DMA_CTCIF6;
        if (_twi_flags & TWI_FLAG_AUTOSTOP) {
            TWI_Wait(I2C_MASTER_BYTE_TRANSMITTED);
            TWI_Stop();
        }
        _twi_flags = 0;
    }
#endif
}

void __attribute__((interrupt("WCH-Interrupt-fast"))) DMA1_Channel7_IRQHandler() {
#ifdef USE_SPL
    if (DMA_GetITStatus(DMA1_IT_TC7)) {
        DMA_Cmd(DMA1_Channel7, DISABLE);
        DMA_ClearITPendingBit(DMA1_IT_TC7);
        if (_twi_flags & TWI_FLAG_AUTOSTOP) {
//            I2C_AcknowledgeConfig(I2C1, DISABLE);
            TWI_Stop();
        }
        _twi_flags = 0;
    }
#else
    if (DMA1->INTFR & DMA_TCIF7) {
        DMA1_Channel7->CFGR &= ~(uint32_t)DMA_CFGR1_EN;
        DMA1->INTFCR = DMA_CTCIF7;
        if (_twi_flags & TWI_FLAG_AUTOSTOP) {
//            I2C1->CTLR1 &= ~I2C_CTLR1_ACK;
            TWI_Stop();
        }
        _twi_flags = 0;
    }
#endif
}
