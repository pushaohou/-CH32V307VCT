/**
 * @file    bluetooth.h
 * @brief   HC-05 蓝牙驱动 — USART2 透传
 */
#ifndef __BLUETOOTH_H
#define __BLUETOOTH_H

#include "guide_cane_config.h"

/* 报警等级 */
typedef enum {
    ALERT_NONE     = 0,
    ALERT_WARNING  = 1,   /* 有障碍 */
    ALERT_DANGER   = 2,   /* 危险距离 */
    ALERT_FALL     = 3    /* 跌倒 */
} BT_AlertLevel;

/* API */
void BT_Init(void);
void BT_SendByte(uint8_t data);
void BT_SendString(const char *str);
void BT_SendData(uint16_t front_cm, uint16_t left_cm, uint16_t right_cm,
                 float pitch, float roll, BT_AlertLevel alert);
void BT_SendAlert(BT_AlertLevel level);

#endif /* __BLUETOOTH_H */
