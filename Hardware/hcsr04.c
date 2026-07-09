/**
 * @file    hcsr04.c
 * @brief   HC-SR04 超声波测距 — 3路 GPIO + TIM2 硬件计时
 * @note    TIM2 配置为 1MHz 自由计数器，1 tick = 1µs，精度 ~1µs
 *          - 60ms Echo 超时
 *          - 脉冲过滤（50μs ~ 35.4ms）
 *          - 三路独立测距，每路间隔 10ms 防串扰
 */
#include "hcsr04.h"
#include "debug.h"
#include "ch32v30x_tim.h"

/* 计时参数（与参考一致） */
#define ECHO_TIMEOUT_US     60000   /* Echo 超时 60ms ≈ 10m 声程 */
#define PULSE_MIN_US        50      /* 最小有效脉宽 */
#define PULSE_MAX_US        35400   /* 最大有效脉宽 ≈ 6m */

/* GPIO 结构体映射 */
typedef struct {
    GPIO_TypeDef *trig_port;
    uint16_t      trig_pin;
    GPIO_TypeDef *echo_port;
    uint16_t      echo_pin;
} HCSR04_Pins;

static const HCSR04_Pins hcsr04_pins[3] = {
    { /* 前方 */
        HCSR04_FRONT_TRIG_PORT, HCSR04_FRONT_TRIG_PIN,
        HCSR04_FRONT_ECHO_PORT, HCSR04_FRONT_ECHO_PIN
    },
    { /* 左侧 */
        HCSR04_LEFT_TRIG_PORT, HCSR04_LEFT_TRIG_PIN,
        HCSR04_LEFT_ECHO_PORT, HCSR04_LEFT_ECHO_PIN
    },
    { /* 右侧 */
        HCSR04_RIGHT_TRIG_PORT, HCSR04_RIGHT_TRIG_PIN,
        HCSR04_RIGHT_ECHO_PORT, HCSR04_RIGHT_ECHO_PIN
    }
};

void HCSR04_Init(void)
{
    GPIO_InitTypeDef GPIO_InitOut = {0};
    GPIO_InitTypeDef GPIO_InitIn  = {0};

    /* 启用 TIM2 作为 100kHz 自由计数器（1 tick = 10µs，16-bit 65ms 范围） */
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);
    {
        TIM_TimeBaseInitTypeDef TIM_InitStruct = {0};
        TIM_InitStruct.TIM_Period        = 0xFFFF;
        TIM_InitStruct.TIM_Prescaler     = (SystemCoreClock / 100000) - 1;  /* 100kHz */
        TIM_InitStruct.TIM_ClockDivision = TIM_CKD_DIV1;
        TIM_InitStruct.TIM_CounterMode   = TIM_CounterMode_Up;
        TIM_TimeBaseInit(TIM2, &TIM_InitStruct);
    }
    TIM_Cmd(TIM2, ENABLE);

    GPIO_InitOut.GPIO_Mode  = GPIO_Mode_Out_PP;
    GPIO_InitOut.GPIO_Speed = GPIO_Speed_50MHz;

    GPIO_InitIn.GPIO_Mode  = GPIO_Mode_IN_FLOATING;

    /* 前方 */
    RCC_APB2PeriphClockCmd(HCSR04_FRONT_TRIG_CLK | HCSR04_FRONT_ECHO_CLK, ENABLE);
    GPIO_InitOut.GPIO_Pin = HCSR04_FRONT_TRIG_PIN;
    GPIO_Init(HCSR04_FRONT_TRIG_PORT, &GPIO_InitOut);
    GPIO_InitIn.GPIO_Pin  = HCSR04_FRONT_ECHO_PIN;
    GPIO_Init(HCSR04_FRONT_ECHO_PORT, &GPIO_InitIn);

    /* 左侧 */
    RCC_APB2PeriphClockCmd(HCSR04_LEFT_TRIG_CLK | HCSR04_LEFT_ECHO_CLK, ENABLE);
    GPIO_InitOut.GPIO_Pin = HCSR04_LEFT_TRIG_PIN;
    GPIO_Init(HCSR04_LEFT_TRIG_PORT, &GPIO_InitOut);
    GPIO_InitIn.GPIO_Pin  = HCSR04_LEFT_ECHO_PIN;
    GPIO_Init(HCSR04_LEFT_ECHO_PORT, &GPIO_InitIn);

    /* 右侧 */
    RCC_APB2PeriphClockCmd(HCSR04_RIGHT_TRIG_CLK | HCSR04_RIGHT_ECHO_CLK, ENABLE);
    GPIO_InitOut.GPIO_Pin = HCSR04_RIGHT_TRIG_PIN;
    GPIO_Init(HCSR04_RIGHT_TRIG_PORT, &GPIO_InitOut);
    GPIO_InitIn.GPIO_Pin  = HCSR04_RIGHT_ECHO_PIN;
    GPIO_Init(HCSR04_RIGHT_ECHO_PORT, &GPIO_InitIn);

    /* 初始所有 Trig 拉低 */
    for (uint8_t i = 0; i < 3; i++) {
        GPIO_ResetBits(hcsr04_pins[i].trig_port, hcsr04_pins[i].trig_pin);
    }
}

static void HCSR04_Trigger(HCSR04_Channel ch)
{
    if (ch > HCSR04_RIGHT) return;

    GPIO_SetBits(hcsr04_pins[ch].trig_port, hcsr04_pins[ch].trig_pin);
    Delay_Us(HCSR04_TRIG_US);
    GPIO_ResetBits(hcsr04_pins[ch].trig_port, hcsr04_pins[ch].trig_pin);
}

uint16_t HCSR04_GetDistance(HCSR04_Channel ch)
{
    uint32_t ticks;
    uint32_t duration_us;
    uint16_t distance_cm;
    /* static uint32_t dbg_cnt = 0; */

    if (ch > HCSR04_RIGHT) return 0;

    const HCSR04_Pins *p = &hcsr04_pins[ch];

    /* 发送 Trig 脉冲 */
    HCSR04_Trigger(ch);

    /* 等待 Echo 上升沿（超时 60ms，TIM2 100kHz = 10µs/tick → 6000 ticks） */
    {
        uint32_t start = TIM2->CNT;
        while (GPIO_ReadInputDataBit(p->echo_port, p->echo_pin) == 0) {
            if ((TIM2->CNT - start) > (ECHO_TIMEOUT_US / 10)) {
                /* if (++dbg_cnt % 10 == 0)
                    printf("HC: ch%d no rising edge\r\n", ch); */
                return 0;
            }
        }
    }

    /* 用 TIM2 测量 Echo 高电平脉宽（100kHz = 10µs/tick） */
    ticks = 0;
    {
        uint32_t start = TIM2->CNT;
        while (GPIO_ReadInputDataBit(p->echo_port, p->echo_pin) == 1) {
            ticks = TIM2->CNT - start;
            if (ticks > (ECHO_TIMEOUT_US / 10)) {
                return 0;
            }
        }
    }

    duration_us = ticks * 10;  /* 10µs/tick → µs */

    /* 脉冲宽度过滤（50μs ~ 35.4ms） */
    if (duration_us < PULSE_MIN_US || duration_us > PULSE_MAX_US) {
        /* if (dbg_cnt % 10 == 0)
            printf("HC: ch%d dur=%lu us filtered\r\n", ch, duration_us); */
        return 0;
    }

    /* 微秒 → 厘米 */
    distance_cm = (uint16_t)(duration_us / 58);

    /* if (dbg_cnt % 10 == 0)
        printf("HC: ch%d dur=%lu us dist=%u cm\r\n", ch, duration_us, distance_cm); */

    return distance_cm;
}

void HCSR04_MeasureAll(HCSR04_Result *result)
{
    if (!result) return;

    result->front_cm = HCSR04_GetDistance(HCSR04_FRONT);
    Delay_Ms(10);  /* 避免相邻探头串扰 */
    result->left_cm  = HCSR04_GetDistance(HCSR04_LEFT);
    Delay_Ms(10);
    result->right_cm = HCSR04_GetDistance(HCSR04_RIGHT);
}
