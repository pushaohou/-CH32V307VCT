/**
 * @file    bluetooth.c
 * @brief   HC-05 蓝牙模块 — USART2 透传
 *          HC-05默认 9600/8/N/1，上电自动进入透传模式
 *          看护端通过蓝牙串口接收传感器数据
 */
#include "bluetooth.h"
#include "debug.h"
#include <stdio.h>

void BT_Init(void)
{
    GPIO_InitTypeDef  GPIO_InitStruct  = {0};
    USART_InitTypeDef USART_InitStruct = {0};

    /* 时钟 — USART2 + 控制引脚 */
    RCC_APB2PeriphClockCmd(BT_USART_TX_CLK | BT_USART_RX_CLK |
                           BT_EN_CLK | BT_STATE_CLK, ENABLE);
    RCC_APB1PeriphClockCmd(BT_USART_CLK, ENABLE);

    /* TX — 复用推挽 */
    GPIO_InitStruct.GPIO_Pin   = BT_USART_TX_PIN;
    GPIO_InitStruct.GPIO_Mode  = GPIO_Mode_AF_PP;
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(BT_USART_TX_PORT, &GPIO_InitStruct);

    /* RX — 浮空输入 */
    GPIO_InitStruct.GPIO_Pin   = BT_USART_RX_PIN;
    GPIO_InitStruct.GPIO_Mode  = GPIO_Mode_IN_FLOATING;
    GPIO_Init(BT_USART_RX_PORT, &GPIO_InitStruct);

    /* EN — 推挽输出 LOW, 确保 HC-05 上电进入透传模式 */
    GPIO_InitStruct.GPIO_Pin   = BT_EN_PIN;
    GPIO_InitStruct.GPIO_Mode  = GPIO_Mode_Out_PP;
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(BT_EN_PORT, &GPIO_InitStruct);
    GPIO_ResetBits(BT_EN_PORT, BT_EN_PIN);

    /* STATE — 浮空输入, HC-05 连接状态 (HIGH=已连接) */
    GPIO_InitStruct.GPIO_Pin   = BT_STATE_PIN;
    GPIO_InitStruct.GPIO_Mode  = GPIO_Mode_IN_FLOATING;
    GPIO_Init(BT_STATE_PORT, &GPIO_InitStruct);

    /* USART2 */
    USART_InitStruct.USART_BaudRate            = BT_BAUDRATE;
    USART_InitStruct.USART_WordLength          = USART_WordLength_8b;
    USART_InitStruct.USART_StopBits            = USART_StopBits_1;
    USART_InitStruct.USART_Parity              = USART_Parity_No;
    USART_InitStruct.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStruct.USART_Mode                = USART_Mode_Tx | USART_Mode_Rx;
    USART_Init(BT_USART, &USART_InitStruct);
    USART_Cmd(BT_USART, ENABLE);
}

void BT_SendByte(uint8_t data)
{
    while (USART_GetFlagStatus(BT_USART, USART_FLAG_TC) == RESET)
        ;
    USART_SendData(BT_USART, data);
}

void BT_SendString(const char *str)
{
    while (*str) {
        BT_SendByte((uint8_t)*str++);
    }
}

/**
 * @brief   发送完整的传感器数据帧
 * @note    格式: "F:xxx L:xxx R:xxx | P:+xx.x R:+xx.x | A:0\r\n"
 *          字段间用空格分隔，JSON风格可自定义
 */
void BT_SendData(uint16_t front_cm, uint16_t left_cm, uint16_t right_cm,
                 float pitch, float roll, BT_AlertLevel alert)
{
    char buf[128];
    const char *alert_str[] = {"OK", "WARN", "DANGER", "FALL"};
    int p = (int)(pitch * 10);
    int r = (int)(roll  * 10);
    char ps = (p < 0) ? '-' : '+'; if (p < 0) p = -p;
    char rs = (r < 0) ? '-' : '+'; if (r < 0) r = -r;

    snprintf(buf, sizeof(buf),
             "F:%03u L:%03u R:%03u P:%c%d.%d R:%c%d.%d %s\r\n",
             front_cm, left_cm, right_cm,
             ps, p / 10, p % 10, rs, r / 10, r % 10,
             alert_str[alert > ALERT_FALL ? ALERT_FALL : alert]);

    BT_SendString(buf);
}

void BT_SendAlert(BT_AlertLevel level)
{
    switch (level) {
        case ALERT_WARNING:
            BT_SendString("ALERT:WARN Obstacle ahead\r\n");
            break;
        case ALERT_DANGER:
            BT_SendString("ALERT:DANGER Too close!\r\n");
            break;
        case ALERT_FALL:
            BT_SendString("ALERT:FALL DETECTED! Emergency!\r\n");
            break;
        default:
            break;
    }
}
