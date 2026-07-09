/**
 * @file    buzzer.h
 * @brief   蜂鸣器驱动 — 5V有源电磁式，低电平触发
 *
 *          一发式方向序列（与参考项目对齐）:
 *          - 前方障碍 → 3声短鸣（150ms ON / 80ms OFF）
 *          - 左侧障碍 → 2声短鸣
 *          - 右侧障碍 → 1声短鸣
 *          - 跌倒     → 长鸣 1500ms
 */
#ifndef __BUZZER_H
#define __BUZZER_H

#include "guide_cane_config.h"

typedef enum {
    BUZZER_OFF   = 0,
    BUZZER_FRONT = 1,  /* 3短鸣 — 前方 */
    BUZZER_LEFT  = 2,  /* 2短鸣 — 左侧 */
    BUZZER_RIGHT = 3,  /* 1短鸣 — 右侧 */
    BUZZER_FALL  = 4   /* 长鸣 1500ms — 跌倒 */
} Buzzer_Pattern;

void Buzzer_Init(void);
void Buzzer_On(void);          /* 直接响（自检用） */
void Buzzer_Off(void);         /* 直接停 */

/**
 * @brief 触发一次鸣响序列（相同模式不重复触发）
 * @param pattern: BUZZER_OFF 停止，其他值触发对应方向序列
 */
void Buzzer_Trigger(Buzzer_Pattern pattern);

/**
 * @brief 非阻塞状态机 — 每ms调用一次
 */
void Buzzer_Process(void);

#endif /* __BUZZER_H */
