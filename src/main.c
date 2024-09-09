//#define ADC_8BIT

#include <string.h>
#include <ch32v00x.h>
#ifdef USE_SPL
#include <ch32v00x_pwr.h>
#include <ch32v00x_rcc.h>
#include <ch32v00x_adc.h>
#include <ch32v00x_gpio.h>
#include <ch32v00x_tim.h>
#endif
#include "utils.h"
#include "twi.h"
#include "ssd1306.h"
#include "encoder.h"

#define ARRAY_SIZE(a)   (sizeof(a) / sizeof(a[0]))

#define CHART_HEIGHT    55
#define CHART_TOP       (OLED_HEIGHT - CHART_HEIGHT)
#define CHART_LEFT      8

#define ADC_DATA_SIZE   120
#define ADC_TRIG_SIZE   (ADC_DATA_SIZE / 2)

#define ADC_FLAG_BUSY   1

enum { TRIG_FREE, TRIG_RISING, TRIG_FALLING };

const uint16_t PERIODS[] = {
    1, 2, 5, 10, 25, 50, 100, 250, 500, 1000, 2500, 5000, 10000
};

#define DEF_PERIOD  3 // 10 us.

#ifdef ADC_8BIT
static uint8_t adc_data[ADC_DATA_SIZE + ADC_TRIG_SIZE];
#else
static uint16_t adc_data[ADC_DATA_SIZE + ADC_TRIG_SIZE];
#endif
static char timestr[8];
static uint16_t vcc; // 500 or 330
static volatile uint8_t _adc_flags;
//static volatile uint8_t adc_len = 0;
static uint8_t period = DEF_PERIOD;
static uint8_t trig = TRIG_RISING;
static uint8_t scale = 0;

static void Init_PVD() {
#ifdef USE_SPL
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);

    PWR_PVDLevelConfig(PWR_PVDLevel_4V4);
    PWR_PVDCmd(ENABLE);
#else
    RCC->APB1PCENR |= RCC_PWREN;
    RCC->APB2PCENR |= RCC_AFIOEN;

//    PWR->CTLR &= ~PWR_CTLR_PLS;
    PWR->CTLR |= (PWR_CTLR_PLS_0 | PWR_CTLR_PLS_1 | PWR_CTLR_PLS_2 | PWR_CTLR_PVDE);
#endif
}

static void Init_ADC(uint16_t us) {
#ifdef USE_SPL
    GPIO_InitTypeDef GPIO_InitStructure = { 0 };
    ADC_InitTypeDef  ADC_InitStructure = { 0 };
    DMA_InitTypeDef DMA_InitStructure = { 0 };
    NVIC_InitTypeDef NVIC_InitStructure = { 0 };

    TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStructure;
    TIM_OCInitTypeDef TIM_OCInitStructure;

    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1 | RCC_APB2Periph_GPIOC, ENABLE);
    RCC_ADCCLKConfig(RCC_PCLK2_Div2); // 24 MHz
    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
    GPIO_Init(GPIOC, &GPIO_InitStructure);

    ADC_DeInit(ADC1);
    ADC_InitStructure.ADC_Mode = ADC_Mode_Independent;
    ADC_InitStructure.ADC_ScanConvMode = DISABLE;
    ADC_InitStructure.ADC_ContinuousConvMode = DISABLE;
    ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_T2_CC1;
#ifdef ADC_8BIT
    ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Left;
#else
    ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;
#endif
    ADC_InitStructure.ADC_NbrOfChannel = 1;
    ADC_Init(ADC1, &ADC_InitStructure);

    ADC_RegularChannelConfig(ADC1, ADC_Channel_2, 1, us > 10 ? ADC_SampleTime_241Cycles : us > 3 ? ADC_SampleTime_73Cycles : us > 2 ? ADC_SampleTime_57Cycles : us > 1 ? ADC_SampleTime_30Cycles : ADC_SampleTime_9Cycles);
    ADC_ExternalTrig_DLY(ADC1, ADC_ExternalTrigRegul_DLY, 0);
    ADC_ExternalTrigConvCmd(ADC1, ENABLE);

    ADC_Cmd(ADC1, ENABLE);

    ADC_Calibration_Vol(ADC1, ADC_CALVOL_50PERCENT);
    ADC_ResetCalibration(ADC1);
    while(ADC_GetResetCalibrationStatus(ADC1)) {}
    ADC_StartCalibration(ADC1);
    while(ADC_GetCalibrationStatus(ADC1)) {}

    ADC_DMACmd(ADC1, ENABLE);

    DMA_DeInit(DMA1_Channel1);
#ifdef ADC_8BIT
    DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)&ADC1->RDATAR + 1;
#else
    DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)&ADC1->RDATAR;
#endif
    DMA_InitStructure.DMA_MemoryBaseAddr = (uint32_t)adc_data;
    DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralSRC;
    DMA_InitStructure.DMA_BufferSize = (trig == TRIG_FREE) ? ADC_DATA_SIZE : ARRAY_SIZE(adc_data);
    DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
    DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
#ifdef ADC_8BIT
    DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
    DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
#else
    DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord;
    DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_HalfWord;
#endif
    DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;
    DMA_InitStructure.DMA_Priority = DMA_Priority_VeryHigh;
    DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;
    DMA_Init(DMA1_Channel1, &DMA_InitStructure);

    NVIC_InitStructure.NVIC_IRQChannel = DMA1_Channel1_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

    DMA_ITConfig(DMA1_Channel1, DMA_IT_TC, ENABLE);
/*
    NVIC_InitStructure.NVIC_IRQChannel = ADC_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

    ADC_ITConfig(ADC1, ADC_IT_EOC, ENABLE);
*/

    TIM_TimeBaseStructInit(&TIM_TimeBaseInitStructure);
    TIM_TimeBaseInitStructure.TIM_Prescaler = (us > 1 ? 48 : 24) - 1; // 0.5/1 us.
    TIM_TimeBaseInitStructure.TIM_Period = (us > 1 ? us : 2) - 1;
    TIM_TimeBaseInit(TIM2, &TIM_TimeBaseInitStructure);

    TIM_OCStructInit(&TIM_OCInitStructure);
    TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;
    TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
    TIM_OCInitStructure.TIM_Pulse = (us > 1 ? us : 2) - 1;
    TIM_OC1Init(TIM2, &TIM_OCInitStructure);

    TIM_CtrlPWMOutputs(TIM2, ENABLE);
    TIM_OC1PreloadConfig(TIM2, TIM_OCPreload_Disable);
    TIM_ARRPreloadConfig(TIM2, ENABLE);
//    TIM_Cmd(TIM2, ENABLE);
#else
    RCC->APB1PCENR |= RCC_TIM2EN;
    RCC->APB2PCENR |= (RCC_ADC1EN | RCC_IOPCEN);
    RCC->AHBPCENR |= RCC_DMA1EN;

    UPDATE_REG32(&RCC->CFGR0, 0xFFFF07FF, 0); // CFGR0_ADCPRE_Reset_Mask, 24 MHz

    UPDATE_REG32(&GPIOC->CFGLR, ~(0x0F << (4 * 4)), 0x00 << (4 * 4));

    RCC->APB2PRSTR |= RCC_ADC1RST;
    RCC->APB2PRSTR &= ~RCC_ADC1RST;

    UPDATE_REG32(&ADC1->CTLR1, 0xFFF0FEFF, 0 | (0 << 8)); // CTLR1_CLEAR_Mask
#ifdef ADC_8BIT
    UPDATE_REG32(&ADC1->CTLR2, 0xFFF1F7FD, ADC_ALIGN | ADC_EXTSEL_2 | (0 << 1) | ADC_EXTTRIG); // CTLR2_CLEAR_Mask
#else
    UPDATE_REG32(&ADC1->CTLR2, 0xFFF1F7FD, ADC_EXTSEL_2 | (0 << 1) | ADC_EXTTRIG); // CTLR2_CLEAR_Mask
#endif
    UPDATE_REG32(&ADC1->RSQR1, 0xFF0FFFFF, (1 - 1) << 20); // RSQR1_CLEAR_Mask
    UPDATE_REG32(&ADC1->SAMPTR2, ~(0x07 << (3 * 2)),
        (uint32_t)(us > 10 ? 0x07 : us > 3 ? 0x06 : us > 2 ? 0x05 : us > 1 ? 0x03 : 0x01) << (3 * 2));
    UPDATE_REG32(&ADC1->RSQR3, ~(0x1F << (5 * 0)), (uint32_t)2 << (5 * 0));
    ADC1->DLYR &= ~(uint32_t)0x03FF;

    ADC1->CTLR2 |= ADC_ADON;

    UPDATE_REG32(&ADC1->CTLR1, ~(uint32_t)(3 << 25), ADC_CALVOLSELECT_0);
    ADC1->CTLR2 |= ADC_RSTCAL;
    while (ADC1->CTLR2 & ADC_RSTCAL) {}
    ADC1->CTLR2 |= ADC_CAL;
    while (ADC1->CTLR2 & ADC_CAL) {}

    ADC1->CTLR2 |= ADC_DMA;

#ifdef ADC_8BIT
    DMA1_Channel1->CFGR = DMA_CFGR1_MINC | DMA_CFGR1_PL_0 | DMA_CFGR1_PL_1;
#else
    DMA1_Channel1->CFGR = DMA_CFGR1_MINC | DMA_CFGR1_PSIZE_0 | DMA_CFGR1_MSIZE_0 | DMA_CFGR1_PL_0 | DMA_CFGR1_PL_1;
#endif
    DMA1_Channel1->CNTR = (trig == TRIG_FREE) ? ADC_DATA_SIZE : ARRAY_SIZE(adc_data);
#ifdef ADC_8BIT
    DMA1_Channel1->PADDR = (uint32_t)&ADC1->RDATAR + 1;
#else
    DMA1_Channel1->PADDR = (uint32_t)&ADC1->RDATAR;
#endif
    DMA1_Channel1->MADDR = (uint32_t)adc_data;

    NVIC_SetPriority(DMA1_Channel1_IRQn, (0 << 7) | (1 << 6));
    NVIC_EnableIRQ(DMA1_Channel1_IRQn);

    DMA1_Channel1->CFGR |= DMA_CFGR1_TCIE;
/*
    NVIC_SetPriority(ADC_IRQn, (0 << 7) | (1 << 6));
    NVIC_EnableIRQ(ADC_IRQn);

    ADC1->CTLR1 |= ADC_EOCIE;
*/

    TIM2->CTLR1 = TIM_ARPE;
    TIM2->ATRLR = (us > 1 ? us : 2) - 1;
    TIM2->PSC = (us > 1 ? 48 : 24) - 1; // 0.5/1 us.
    TIM2->SWEVGR = TIM_UG;

    UPDATE_REG16(&TIM2->CCER, ~(TIM_CC1E | TIM_CC1P), TIM_CC1E);
    UPDATE_REG16(&TIM2->CHCTLR1, ~(TIM_OC1M | TIM_CC1S | TIM_OC1PE), TIM_OC1M_1 | TIM_OC1M_2);
    TIM2->CH1CVR = (us > 1 ? us : 2) - 1;

    TIM2->BDTR |= TIM_MOE;
//    TIM2->CTLR1 |= TIM_CEN;
#endif
}

static void Reinit_ADC(uint16_t us) {
#ifdef USE_SPL
    TIM_Cmd(TIM2, DISABLE);
    DMA_Cmd(DMA1_Channel1, DISABLE);
    ADC_Cmd(ADC1, DISABLE);

    ADC_RegularChannelConfig(ADC1, ADC_Channel_2, 1, us > 10 ? ADC_SampleTime_241Cycles : us > 3 ? ADC_SampleTime_73Cycles : us > 2 ? ADC_SampleTime_57Cycles : us > 1 ? ADC_SampleTime_30Cycles : ADC_SampleTime_9Cycles);

    ADC_Cmd(ADC1, ENABLE);

/*
#ifdef ADC_8BIT
    if (scale && (us > 10)) { // 8 low bits
        ADC1->CTLR2 &= ~ADC_ALIGN;
        DMA1_Channel1->PADDR = (uint32_t)&ADC1->RDATAR;
    } else { // 8 high bits
        ADC1->CTLR2 |= ADC_ALIGN;
        DMA1_Channel1->PADDR = (uint32_t)&ADC1->RDATAR + 1;
    }
#endif
*/

    TIM_SetAutoreload(TIM2, (us > 1 ? us : 2) - 1);
    TIM_PrescalerConfig(TIM2, (us > 1 ? 48 : 24) - 1, TIM_PSCReloadMode_Immediate); // 0.5/1 us.

    TIM_SetCompare1(TIM2, (us > 1 ? us : 2) - 1);
#else
    TIM2->CTLR1 &= ~TIM_CEN;
    DMA1_Channel1->CFGR &= ~(uint32_t)DMA_CFGR1_EN;
    ADC1->CTLR2 &= ~ADC_ADON;

/*
#ifdef ADC_8BIT
    if (scale && (us > 10)) { // 8 low bits
        ADC1->CTLR2 &= ~ADC_ALIGN;
        DMA1_Channel1->PADDR = (uint32_t)&ADC1->RDATAR;
    } else { // 8 high bits
        ADC1->CTLR2 |= ADC_ALIGN;
        DMA1_Channel1->PADDR = (uint32_t)&ADC1->RDATAR + 1;
    }
#endif
*/

    UPDATE_REG32(&ADC1->SAMPTR2, ~(0x07 << (3 * 2)),
        (uint32_t)(us > 10 ? 0x07 : us > 3 ? 0x06 : us > 2 ? 0x05 : us > 1 ? 0x03 : 0x01) << (3 * 2));
    UPDATE_REG32(&ADC1->RSQR3, ~(0x1F << (5 * 0)), (uint32_t)2 << (5 * 0));

    ADC1->CTLR2 |= ADC_ADON;

    TIM2->ATRLR = (us > 1 ? us : 2) - 1;
    TIM2->PSC = (us > 1 ? 48 : 24) - 1; // 0.5/1 us.
    TIM2->SWEVGR = TIM_UG;

    TIM2->CH1CVR = (us > 1 ? us : 2) - 1;
#endif
}

static void Start_ADC(bool wait) {
    DMA1_Channel1->MADDR = (uint32_t)adc_data;
#ifdef USE_SPL
    DMA_SetCurrDataCounter(DMA1_Channel1, (trig == TRIG_FREE) ? ADC_DATA_SIZE : ARRAY_SIZE(adc_data));
    DMA_Cmd(DMA1_Channel1, ENABLE);

    TIM_SetCounter(TIM2, 0);
//    ADC_ExternalTrigConvCmd(ADC1, ENABLE);
//    adc_len = 0;
    _adc_flags = ADC_FLAG_BUSY;
    TIM_Cmd(TIM2, ENABLE);
#else
    DMA1_Channel1->CNTR = (trig == TRIG_FREE) ? ADC_DATA_SIZE : ARRAY_SIZE(adc_data);
    DMA1_Channel1->CFGR |= DMA_CFGR1_EN;

    TIM2->CNT = 0;
//    ADC1->CTLR2 |= ADC_EXTTRIG;
//    adc_len = 0;
    _adc_flags = ADC_FLAG_BUSY;
    TIM2->CTLR1 |= TIM_CEN;
#endif
    if (wait) {
//        while (adc_len < ADC_DATA_SIZE) {}
        while (_adc_flags & ADC_FLAG_BUSY) {}
    }
}

static char *u2str(char *s, uint16_t value, uint16_t dec) {
    uint16_t divider;

    for (divider = 10000; divider > dec; divider /= 10) {
        if (value / divider)
            break;
    }
    for (; divider > 0; divider /= 10) {
        *s++ = '0' + value / divider;
        value %= divider;
    }
    *s = '\0';
    return s;
}

static void norm_time(uint32_t period) {
    char *s;

    period *= (ADC_DATA_SIZE >> 2);
    if (period < 1000) {
//        sprintf(timestr, "%uu", period);
        s = u2str(timestr, period, 1);
        *s++ = 'u';
    } else {
        if ((period % 1000) == 0) {
//            sprintf(timestr, "%um", period / 1000);
            s = u2str(timestr, period / 1000, 1);
        } else {
//            sprintf(timestr, "%u.%um", period / 1000, (period % 1000) / 100);
            s = u2str(timestr, period / 1000, 1);
            *s++ = '.';
            s = u2str(s, (period % 1000) / 100, 1);
        }
        *s++ = 'm';
    }
    *s = '\0';
}

static inline uint16_t trim(uint16_t data) {
#ifdef ADC_8BIT
    return (data > 255) ? 255 : data & 0xFF;
#else
    return (data > 1023) ? 1023 : data & 0x3FF;
#endif
}

static void find_range(uint16_t *min_value, uint16_t *max_value, uint8_t start, uint8_t size) {
    *min_value = adc_data[start];
    *max_value = adc_data[start];
    ++start;

    while (--size) {
        if (adc_data[start] < *min_value)
            *min_value = adc_data[start];
        if (adc_data[start] > *max_value)
            *max_value = adc_data[start];
        ++start;
    }
}

static void draw_screen(bool plot, bool wait) {
    char str[22];
    char *s;
    uint16_t adc_min, adc_max;
#ifdef ADC_8BIT
    uint8_t trig_value;
#else
    uint16_t trig_value;
#endif
    uint8_t trig_start = 0;

    oled_clear();
    oled_vline(CHART_LEFT - 1, CHART_TOP, CHART_HEIGHT, 1);
    if (vcc == 500) { // 5.0 V
        for (uint8_t y = 5 + CHART_TOP; y < OLED_HEIGHT; y += 11) {
            for (uint8_t x = 4; x < OLED_WIDTH; x += 8) {
                oled_hline(x, y, 3, 1);
            }
        }
    } else { // 3.3 V
        for (uint8_t y = 10 + CHART_TOP; y < OLED_HEIGHT; y += 17) {
            for (uint8_t x = 4; x < OLED_WIDTH; x += 8) {
                oled_hline(x, y, 3, 1);
            }
        }
    }
    for (uint8_t x = CHART_LEFT + 30; x < OLED_WIDTH; x += 30) {
        for (uint8_t y = CHART_TOP + 2; y < OLED_HEIGHT; y += 8) {
            oled_vline(x, y, 3, 1);
        }
    }

    if (trig != TRIG_FREE) {
        find_range(&adc_min, &adc_max, 0, ADC_TRIG_SIZE);
        trig_value = (adc_max - adc_min + 1) >> 1; // /2
        while (trig_start < ADC_TRIG_SIZE) {
            if (trig == TRIG_RISING) {
                if ((adc_data[trig_start] < trig_value) && (adc_data[trig_start + 1] >= trig_value))
                    break;
            } else { // trig == TRIG_FALLING
                if ((adc_data[trig_start] > trig_value) && (adc_data[trig_start + 1] <= trig_value))
                    break;
            }
            ++trig_start;
        }
    }
    find_range(&adc_min, &adc_max, trig_start, ADC_DATA_SIZE);
#ifdef ADC_8BIT
    if ((adc_max - adc_min) << scale >= 255)
        trig_value = 0;
    else
        trig_value = (255 - ((adc_max - adc_min) << scale)) >> 1;
#else
    if ((adc_max - adc_min) << scale >= 1023)
        trig_value = 0;
    else
        trig_value = (1023 - ((adc_max - adc_min) << scale)) >> 1;
#endif
    for (uint8_t i = 0; i < ADC_DATA_SIZE; ++i) {
        uint8_t index = i + trig_start;

#ifdef ADC_8BIT
        adc_data[index] = OLED_HEIGHT - 1 - trim(((adc_data[index] - adc_min) << scale) + trig_value) * CHART_HEIGHT / 255;
#else
        adc_data[index] = OLED_HEIGHT - 1 - trim(((adc_data[index] - adc_min) << scale) + trig_value) * CHART_HEIGHT / 1023;
#endif
        if (plot) {
            oled_pixel(i + CHART_LEFT, adc_data[index], 1);
        } else {
            if (i)
                oled_line(i + CHART_LEFT - 1, adc_data[index - 1], i + CHART_LEFT, adc_data[index], 1);
        }
    }
#ifdef ADC_8BIT
    adc_min = adc_min * vcc / 255;
    adc_max = adc_max * vcc / 255;
#else
    adc_min = adc_min * vcc / 1023;
    adc_max = adc_max * vcc / 1023;
#endif
    str[0] = (trig == TRIG_RISING) ? '/' : (trig == TRIG_FALLING) ? '\\' : '~';
    str[1] = ' ';
    str[2] = 'x';
    str[3] = '0' + (1 << scale);
    str[4] = ' ';
    strcpy(&str[5], timestr);
//    sprintf(&str[strlen(str)], "s %u.%02u-%u.%02u", adc_min / 100, adc_min % 100, adc_max / 100, adc_max % 100);
    s = &str[strlen(str)];
    *s++ = 's';
    *s++ = ' ';
    s = u2str(s, adc_min / 100, 1);
    *s++ = '.';
    s = u2str(s, adc_min % 100, 10);
    *s++ = '-';
    s = u2str(s, adc_max / 100, 1);
    *s++ = '.';
    s = u2str(s, adc_max % 100, 10);
    *s = '\0';
    oled_printstr(str, 0, 0, 1);
    oled_flush(wait);
}

int main(void) {
    Init_PVD();

    TWI_Init(400000);

    Encoder_Init(); // To prevent phantom button click, initialize before delay

    delay_ms(50);

    if (! oled_begin()) {
        while (1) {}
    }

    Init_ADC(PERIODS[period]);
    Start_ADC(false);

    norm_time(PERIODS[period]);

#ifdef USE_SPL
    if (PWR_GetFlagStatus(PWR_FLAG_PVDO)) // Below 4.4 V
#else
    if (PWR->CSR & PWR_CSR_PVDO) // Below 4.4 V
#endif
        vcc = 330; // 3.3 V
    else
        vcc = 500; // 5.0 V

    while(1) {
        int8_t e;

        e = Encoder_Read();
        if (e != 0) {
            if (Encoder_Button()) { // Trigger mode or scale
                if (e < 0) { // Trigger mode
                    if (trig < TRIG_FALLING)
                        ++trig;
                    else
                        trig = TRIG_FREE;
                } else { // Scale
                    if (scale < 2) // x4
                        ++scale;
                    else
                        scale = 0;
                }
            } else { // Period
                if (e < 0) {
                    if (period > 0)
                        --period;
                    else
                        period = ARRAY_SIZE(PERIODS) - 1;
                } else {
                    if (period < ARRAY_SIZE(PERIODS) - 1)
                        ++period;
                    else
                        period = 0;
                }
                norm_time(PERIODS[period]);
            }

            Reinit_ADC(PERIODS[period]);
            Start_ADC(false);
        }

        if (! (_adc_flags & ADC_FLAG_BUSY)) {
            draw_screen(false, PERIODS[period] <= 100);
            Start_ADC(false);
        }
    }
}

void __attribute__((interrupt("WCH-Interrupt-fast"))) DMA1_Channel1_IRQHandler() {
#ifdef USE_SPL
    if (DMA_GetITStatus(DMA1_IT_TC1)) {
        TIM_Cmd(TIM2, DISABLE);
        DMA_Cmd(DMA1_Channel1, DISABLE);
        DMA_ClearITPendingBit(DMA1_IT_TC1);
        _adc_flags = 0;
    }
#else
    if (DMA1->INTFR & DMA_TCIF1) {
        TIM2->CTLR1 &= ~TIM_CEN;
        DMA1_Channel1->CFGR &= ~(uint32_t)DMA_CFGR1_EN;
        DMA1->INTFCR = DMA_CTCIF1;
        _adc_flags = 0;
    }
#endif
}

/*
void __attribute__((interrupt("WCH-Interrupt-fast"))) ADC1_IRQHandler() {
#ifdef USE_SPL
    if (ADC_GetITStatus(ADC1, ADC_IT_EOC)) {
#ifdef ADC_8BIT
        adc_data[adc_len] = ADC_GetConversionValue(ADC1) >> 8;
#else
        adc_data[adc_len] = ADC_GetConversionValue(ADC1);
#endif
        if (++adc_len >= ADC_DATA_SIZE) {
//            ADC_ExternalTrigConvCmd(ADC1, DISABLE);
            TIM_Cmd(TIM2, DISABLE);
        }
        ADC_ClearITPendingBit(ADC1, ADC_IT_EOC);
    }
#else
    if (ADC1->STATR & ADC_EOC) {
#ifdef ADC_8BIT
        adc_data[adc_len] = ADC1->RDATAR >> 8;
#else
        adc_data[adc_len] = ADC1->RDATAR;
#endif
        if (++adc_len >= ADC_DATA_SIZE) {
//            ADC1->CTLR2 &= 0xFFEFFFFF; // CTLR2_EXTTRIG_Reset
            TIM2->CTLR1 &= ~TIM_CEN;
        }
        ADC1->STATR = ~(uint32_t)ADC_EOC;
    }
#endif
}
*/
