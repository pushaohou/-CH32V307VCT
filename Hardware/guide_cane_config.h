/**
 * @file    guide_cane_config.h
 * @brief   智能导盲杖 — 引脚配置
 * @note    所有引脚宏定义集中于此，已与扩展板实物确认
 */
#ifndef __GUIDE_CANE_CONFIG_H
#define __GUIDE_CANE_CONFIG_H

#include "ch32v30x_gpio.h"
#include "ch32v30x_rcc.h"

/*===========================================================================
 * OLED SSD1306 — I2C1
 * 7-bit 地址 = 0x3C（发送时左移1位即 0x78）
 *===========================================================================*/
#define OLED_I2C                    I2C1
#define OLED_I2C_CLK                RCC_APB1Periph_I2C1
#define OLED_I2C_SCL_PORT           GPIOB
#define OLED_I2C_SCL_PIN            GPIO_Pin_6
#define OLED_I2C_SCL_CLK            RCC_APB2Periph_GPIOB
#define OLED_I2C_SDA_PORT           GPIOB
#define OLED_I2C_SDA_PIN            GPIO_Pin_7
#define OLED_I2C_SDA_CLK            RCC_APB2Periph_GPIOB

#define OLED_RES_PORT               GPIOB
#define OLED_RES_PIN                GPIO_Pin_8
#define OLED_RES_CLK                RCC_APB2Periph_GPIOB

#define OLED_I2C_ADDR               0x3C   /* 7-bit address, 8-bit write = 0x78 */
#define OLED_I2C_SPEED              400000 /* 400kHz fast mode */

/*===========================================================================
 * MPU6050 — I2C2
 * 7-bit 地址 = 0x68
 *===========================================================================*/
#define MPU6050_I2C                 I2C2
#define MPU6050_I2C_CLK             RCC_APB1Periph_I2C2
#define MPU6050_I2C_SCL_PORT        GPIOB
#define MPU6050_I2C_SCL_PIN         GPIO_Pin_10
#define MPU6050_I2C_SCL_CLK         RCC_APB2Periph_GPIOB
#define MPU6050_I2C_SDA_PORT        GPIOB
#define MPU6050_I2C_SDA_PIN         GPIO_Pin_11
#define MPU6050_I2C_SDA_CLK         RCC_APB2Periph_GPIOB

#define MPU6050_I2C_ADDR            0x68
#define MPU6050_I2C_SPEED           400000

/* MPU6050 辅助引脚 */
#define MPU6050_INT_PORT            GPIOA
#define MPU6050_INT_PIN             GPIO_Pin_15
#define MPU6050_INT_CLK             RCC_APB2Periph_GPIOA
/* AD0 硬件接 GND → 7-bit 地址 = 0x68 */

/*===========================================================================
 * HC-SR04 超声波 ×3（6个GPIO：每路 Trig输出 + Echo输入）
 *===========================================================================*/
/* 前方 */
#define HCSR04_FRONT_TRIG_PORT      GPIOA
#define HCSR04_FRONT_TRIG_PIN       GPIO_Pin_0
#define HCSR04_FRONT_TRIG_CLK       RCC_APB2Periph_GPIOA
#define HCSR04_FRONT_ECHO_PORT      GPIOA
#define HCSR04_FRONT_ECHO_PIN       GPIO_Pin_1
#define HCSR04_FRONT_ECHO_CLK       RCC_APB2Periph_GPIOA

/* 左侧 */
#define HCSR04_LEFT_TRIG_PORT       GPIOA
#define HCSR04_LEFT_TRIG_PIN        GPIO_Pin_4
#define HCSR04_LEFT_TRIG_CLK        RCC_APB2Periph_GPIOA
#define HCSR04_LEFT_ECHO_PORT       GPIOA
#define HCSR04_LEFT_ECHO_PIN        GPIO_Pin_5
#define HCSR04_LEFT_ECHO_CLK        RCC_APB2Periph_GPIOA

/* 右侧 */
#define HCSR04_RIGHT_TRIG_PORT      GPIOA
#define HCSR04_RIGHT_TRIG_PIN       GPIO_Pin_6
#define HCSR04_RIGHT_TRIG_CLK       RCC_APB2Periph_GPIOA
#define HCSR04_RIGHT_ECHO_PORT      GPIOA
#define HCSR04_RIGHT_ECHO_PIN       GPIO_Pin_7
#define HCSR04_RIGHT_ECHO_CLK       RCC_APB2Periph_GPIOA

/* 测距参数 */
#define HCSR04_TIMEOUT_US           30000  /* Echo超时 30ms ≈ 5m */
#define HCSR04_TRIG_US              50     /* Trig脉冲宽度（CS100A 需要 50µs）*/

/*===========================================================================
 * 蜂鸣器 ×1 — 5V电磁式有源蜂鸣器, MOS管驱动（低电平触发，GPIO=0响）
 * 扩展板信号 BEE_IO → 核心板 P2.3 → MCU PC13
 *===========================================================================*/
#define BUZZER_PORT                 GPIOC
#define BUZZER_PIN                  GPIO_Pin_13
#define BUZZER_CLK                  RCC_APB2Periph_GPIOC

/*===========================================================================
 * HC-05 蓝牙 — USART2
 *===========================================================================*/
#define BT_USART                    USART2
#define BT_USART_CLK                RCC_APB1Periph_USART2
#define BT_USART_TX_PORT            GPIOA
#define BT_USART_TX_PIN             GPIO_Pin_2
#define BT_USART_TX_CLK             RCC_APB2Periph_GPIOA
#define BT_USART_RX_PORT            GPIOA
#define BT_USART_RX_PIN             GPIO_Pin_3
#define BT_USART_RX_CLK             RCC_APB2Periph_GPIOA

#define BT_BAUDRATE                 9600

/* HC-05 状态和控制引脚 */
#define BT_STATE_PORT               GPIOB
#define BT_STATE_PIN                GPIO_Pin_0
#define BT_STATE_CLK                RCC_APB2Periph_GPIOB
#define BT_EN_PORT                  GPIOB
#define BT_EN_PIN                   GPIO_Pin_2
#define BT_EN_CLK                   RCC_APB2Periph_GPIOB

/*===========================================================================
 * 系统参数
 *===========================================================================*/
#define OBSTACLE_NEAR_CM            20   /* 近距离阈值（<20cm 危险） */
#define OBSTACLE_MID_CM             50   /* 中距离阈值（<50cm 警告） */
#define FALL_TILT_ANGLE             60   /* 跌倒倾斜角 (度) */
#define FALL_ACCEL_THRESHOLD        2000 /* 跌倒加速度阈值 (mg) */

#endif /* __GUIDE_CANE_CONFIG_H */
