/**
 * @file    buzzer.c
 * @brief   蜂鸣器驱动 — 5V有源电磁式，低电平触发
 *
 *          方向序列（有障碍循环鸣响，周期间有停顿以便分辨方向）:
 *          - 前方 3短鸣 + 停600ms + 循环
 *          - 左侧 2短鸣 + 停600ms + 循环
 *          - 右侧 1短鸣 + 停600ms + 循环
 *          - 跌倒 长鸣 1500ms + 停600ms + 循环
 *
 *          有障碍→循环，无障碍→BUZZER_OFF 立即停。
 */
#include "buzzer.h"

/* 时序参数（与参考一致） */
#define BEEP_ON_MS   150
#define BEEP_GAP_MS  80
#define FALL_ON_MS   1500
#define CYCLE_GAP_MS 600   /* 一轮结束后静音间隔，方便盲人分辨方向 */

static Buzzer_Pattern g_pattern   = BUZZER_OFF;
static uint8_t        g_active    = 0;    /* 序列进行中 */
static uint8_t        g_beep_on   = 1;    /* 当前在 ON 还是 OFF 间隙 */
static uint8_t        g_beep_idx  = 0;    /* 当前第几声（0-based） */
static uint8_t        g_cycle_wait = 0;   /* 正在周期间隔静音中 */
static uint32_t       g_phase_start_ms = 0;
static uint32_t       g_sys_ms   = 0;

void Buzzer_On(void)
{
    GPIO_ResetBits(BUZZER_PORT, BUZZER_PIN);
}

void Buzzer_Off(void)
{
    GPIO_SetBits(BUZZER_PORT, BUZZER_PIN);
}

void Buzzer_Init(void)
{
    GPIO_InitTypeDef GPIO_InitOut = {0};

    RCC_APB2PeriphClockCmd(BUZZER_CLK, ENABLE);

    GPIO_InitOut.GPIO_Pin   = BUZZER_PIN;
    GPIO_InitOut.GPIO_Mode  = GPIO_Mode_Out_PP;
    GPIO_InitOut.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(BUZZER_PORT, &GPIO_InitOut);

    Buzzer_Off();
}

void Buzzer_Trigger(Buzzer_Pattern pattern)
{
    if (pattern == BUZZER_OFF) {
        g_active  = 0;
        g_pattern = BUZZER_OFF;
        Buzzer_Off();
        return;
    }

    /* 相同模式且序列还在跑，不打断 */
    if (pattern == g_pattern && g_active) {
        return;
    }

    /* 新模式，或上一轮已跑完需要循环：重开序列 */
    g_pattern   = pattern;
    g_active    = 1;
    g_beep_idx  = 0;
    g_beep_on   = 1;
    g_cycle_wait = 0;
    g_phase_start_ms = g_sys_ms;
}

void Buzzer_Process(void)
{
    uint32_t elapsed;
    uint8_t  total_beeps;

    g_sys_ms++;

    if (!g_active) return;

    elapsed = g_sys_ms - g_phase_start_ms;

    /*---- 跌倒：长鸣 + 间隔 + 循环 ----*/
    if (g_pattern == BUZZER_FALL) {
        Buzzer_On();
        if (elapsed >= FALL_ON_MS) {
            Buzzer_Off();
            g_cycle_wait = 1;
            g_phase_start_ms = g_sys_ms;
        }
        return;
    }

    /*---- 方向障碍：短鸣序列 ----*/
    if (g_pattern == BUZZER_FRONT)       total_beeps = 3;
    else if (g_pattern == BUZZER_LEFT)   total_beeps = 2;
    else                                 total_beeps = 1;  /* BUZZER_RIGHT */

    /* 周期间隔静音中？ */
    if (g_cycle_wait) {
        Buzzer_Off();
        if (elapsed >= CYCLE_GAP_MS) {
            /* 间隔结束，重新开始一轮 */
            g_beep_idx   = 0;
            g_beep_on    = 1;
            g_cycle_wait = 0;
            g_phase_start_ms = g_sys_ms;
        }
        return;
    }

    if (g_beep_idx >= total_beeps) {
        /* 一轮跑完，进入周期间隔 */
        Buzzer_Off();
        g_cycle_wait = 1;
        g_phase_start_ms = g_sys_ms;
        return;
    }

    if (g_beep_on) {
        /* ON 阶段 */
        Buzzer_On();
        if (elapsed >= BEEP_ON_MS) {
            g_beep_on = 0;
            g_phase_start_ms = g_sys_ms;
        }
    } else {
        /* OFF 间隙 */
        Buzzer_Off();
        if (elapsed >= BEEP_GAP_MS) {
            g_beep_idx++;
            g_beep_on = 1;
            g_phase_start_ms = g_sys_ms;
        }
    }
}
