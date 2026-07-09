/**
 * @file    hcsr04.h
 * @brief   HC-SR04 超声波测距驱动（3路: 前/左/右）
 * @note    Trig≥10μs高脉冲触发，Echo高电平时间=距离×58(μs/cm)
 */
#ifndef __HCSR04_H
#define __HCSR04_H

#include "guide_cane_config.h"

/* 传感器通道 */
typedef enum {
    HCSR04_FRONT = 0,
    HCSR04_LEFT  = 1,
    HCSR04_RIGHT = 2
} HCSR04_Channel;

/* 测距结果 (cm) */
typedef struct {
    uint16_t front_cm;
    uint16_t left_cm;
    uint16_t right_cm;
} HCSR04_Result;

/* API */
void     HCSR04_Init(void);
uint16_t HCSR04_GetDistance(HCSR04_Channel ch);
void     HCSR04_MeasureAll(HCSR04_Result *result);

#endif /* __HCSR04_H */
