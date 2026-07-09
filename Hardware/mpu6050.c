/**
 * @file    mpu6050.c
 * @brief   MPU6050 六轴传感器 — I2C2 硬件I2C驱动
 *          GY-521模块, 7-bit I2C地址 0x68
 */
#include "mpu6050.h"
#include "debug.h"
#include <math.h>

/* MPU6050 寄存器地址 */
#define MPU6050_REG_SMPLRT_DIV    0x19
#define MPU6050_REG_CONFIG         0x1A
#define MPU6050_REG_GYRO_CONFIG    0x1B
#define MPU6050_REG_ACCEL_CONFIG   0x1C
#define MPU6050_REG_ACCEL_XOUT_H   0x3B
#define MPU6050_REG_TEMP_OUT_H     0x41
#define MPU6050_REG_GYRO_XOUT_H    0x43
#define MPU6050_REG_PWR_MGMT_1     0x6B
#define MPU6050_REG_WHO_AM_I       0x75

/* 加速度灵敏度: ±2g → 16384 LSB/g */
#define ACCEL_SENSITIVITY          16384.0f

/* 陀螺仪灵敏度: ±2000°/s → 16.4 LSB/(°/s) */
#define GYRO_SENSITIVITY           16.4f

/* I2C 读写辅助 */
static void MPU6050_I2C_WriteByte(uint8_t reg, uint8_t data)
{
    while (I2C_GetFlagStatus(MPU6050_I2C, I2C_FLAG_BUSY))
        ;

    I2C_GenerateSTART(MPU6050_I2C, ENABLE);
    while (!I2C_CheckEvent(MPU6050_I2C, I2C_EVENT_MASTER_MODE_SELECT))
        ;

    I2C_Send7bitAddress(MPU6050_I2C, MPU6050_I2C_ADDR << 1, I2C_Direction_Transmitter);
    while (!I2C_CheckEvent(MPU6050_I2C, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED))
        ;

    I2C_SendData(MPU6050_I2C, reg);
    while (!I2C_GetFlagStatus(MPU6050_I2C, I2C_FLAG_TXE))
        ;

    I2C_SendData(MPU6050_I2C, data);
    while (!I2C_GetFlagStatus(MPU6050_I2C, I2C_FLAG_TXE))
        ;

    while (!I2C_CheckEvent(MPU6050_I2C, I2C_EVENT_MASTER_BYTE_TRANSMITTED))
        ;

    I2C_GenerateSTOP(MPU6050_I2C, ENABLE);
}

static uint8_t MPU6050_I2C_ReadByte(uint8_t reg)
{
    uint8_t data;

    while (I2C_GetFlagStatus(MPU6050_I2C, I2C_FLAG_BUSY))
        ;

    /* 先写寄存器地址 */
    I2C_GenerateSTART(MPU6050_I2C, ENABLE);
    while (!I2C_CheckEvent(MPU6050_I2C, I2C_EVENT_MASTER_MODE_SELECT))
        ;

    I2C_Send7bitAddress(MPU6050_I2C, MPU6050_I2C_ADDR << 1, I2C_Direction_Transmitter);
    while (!I2C_CheckEvent(MPU6050_I2C, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED))
        ;

    I2C_SendData(MPU6050_I2C, reg);
    while (!I2C_GetFlagStatus(MPU6050_I2C, I2C_FLAG_TXE))
        ;

    while (!I2C_CheckEvent(MPU6050_I2C, I2C_EVENT_MASTER_BYTE_TRANSMITTED))
        ;

    /* 再读数据 */
    I2C_GenerateSTART(MPU6050_I2C, ENABLE);
    while (!I2C_CheckEvent(MPU6050_I2C, I2C_EVENT_MASTER_MODE_SELECT))
        ;

    I2C_Send7bitAddress(MPU6050_I2C, MPU6050_I2C_ADDR << 1, I2C_Direction_Receiver);
    while (!I2C_CheckEvent(MPU6050_I2C, I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED))
        ;

    /* 单字节读，不应答 */
    I2C_AcknowledgeConfig(MPU6050_I2C, DISABLE);
    while (!I2C_GetFlagStatus(MPU6050_I2C, I2C_FLAG_RXNE))
        ;
    data = I2C_ReceiveData(MPU6050_I2C);

    I2C_GenerateSTOP(MPU6050_I2C, ENABLE);
    I2C_AcknowledgeConfig(MPU6050_I2C, ENABLE);  /* 恢复应答 */

    return data;
}

static void MPU6050_I2C_ReadMulti(uint8_t reg, uint8_t *buf, uint8_t len)
{
    uint8_t i;

    while (I2C_GetFlagStatus(MPU6050_I2C, I2C_FLAG_BUSY))
        ;

    /* 写起始地址 */
    I2C_GenerateSTART(MPU6050_I2C, ENABLE);
    while (!I2C_CheckEvent(MPU6050_I2C, I2C_EVENT_MASTER_MODE_SELECT))
        ;

    I2C_Send7bitAddress(MPU6050_I2C, MPU6050_I2C_ADDR << 1, I2C_Direction_Transmitter);
    while (!I2C_CheckEvent(MPU6050_I2C, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED))
        ;

    I2C_SendData(MPU6050_I2C, reg);
    while (!I2C_GetFlagStatus(MPU6050_I2C, I2C_FLAG_TXE))
        ;

    while (!I2C_CheckEvent(MPU6050_I2C, I2C_EVENT_MASTER_BYTE_TRANSMITTED))
        ;

    /* 连续读 */
    I2C_GenerateSTART(MPU6050_I2C, ENABLE);
    while (!I2C_CheckEvent(MPU6050_I2C, I2C_EVENT_MASTER_MODE_SELECT))
        ;

    I2C_Send7bitAddress(MPU6050_I2C, MPU6050_I2C_ADDR << 1, I2C_Direction_Receiver);
    while (!I2C_CheckEvent(MPU6050_I2C, I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED))
        ;

    for (i = 0; i < len; i++) {
        if (i == len - 1) {
            I2C_AcknowledgeConfig(MPU6050_I2C, DISABLE);  /* 最后一个字节不应答 */
        }
        while (!I2C_GetFlagStatus(MPU6050_I2C, I2C_FLAG_RXNE))
            ;
        buf[i] = I2C_ReceiveData(MPU6050_I2C);
    }

    I2C_GenerateSTOP(MPU6050_I2C, ENABLE);
    I2C_AcknowledgeConfig(MPU6050_I2C, ENABLE);
}

uint8_t MPU6050_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    I2C_InitTypeDef  I2C_InitStruct  = {0};

    /* 时钟 */
    RCC_APB2PeriphClockCmd(MPU6050_I2C_SCL_CLK | MPU6050_I2C_SDA_CLK, ENABLE);
    RCC_APB1PeriphClockCmd(MPU6050_I2C_CLK, ENABLE);

    /* SCL/SDA — 复用开漏 */
    GPIO_InitStruct.GPIO_Pin   = MPU6050_I2C_SCL_PIN | MPU6050_I2C_SDA_PIN;
    GPIO_InitStruct.GPIO_Mode  = GPIO_Mode_AF_OD;
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(MPU6050_I2C_SCL_PORT, &GPIO_InitStruct);

    /* 复位 I2C2 总线（清除可能卡死的状态，与 OLED I2C1 同理） */
    I2C_Cmd(MPU6050_I2C, DISABLE);

    /* I2C */
    I2C_InitStruct.I2C_ClockSpeed          = MPU6050_I2C_SPEED;
    I2C_InitStruct.I2C_Mode                = I2C_Mode_I2C;
    I2C_InitStruct.I2C_DutyCycle           = I2C_DutyCycle_16_9;
    I2C_InitStruct.I2C_OwnAddress1         = 0x00;
    I2C_InitStruct.I2C_Ack                 = I2C_Ack_Enable;
    I2C_InitStruct.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit;
    I2C_Init(MPU6050_I2C, &I2C_InitStruct);
    I2C_Cmd(MPU6050_I2C, ENABLE);

    /* 验证设备 */
    if (MPU6050_WhoAmI() != 0x68) {
        return 0;
    }

    /* 唤醒 + 配置 */
    MPU6050_I2C_WriteByte(MPU6050_REG_PWR_MGMT_1, 0x00);     /* 退出睡眠 */
    Delay_Ms(100);

    MPU6050_I2C_WriteByte(MPU6050_REG_SMPLRT_DIV, 0x07);     /* 采样率 = 1kHz/(1+7) = 125Hz */
    MPU6050_I2C_WriteByte(MPU6050_REG_CONFIG, 0x06);          /* DLPF = 5Hz */
    MPU6050_I2C_WriteByte(MPU6050_REG_GYRO_CONFIG, 0x18);    /* 陀螺仪 ±2000°/s */
    MPU6050_I2C_WriteByte(MPU6050_REG_ACCEL_CONFIG, 0x00);   /* 加速度 ±2g */

    return 1;
}

uint8_t MPU6050_WhoAmI(void)
{
    return MPU6050_I2C_ReadByte(MPU6050_REG_WHO_AM_I);
}

uint8_t MPU6050_ReadAll(MPU6050_Data *data)
{
    uint8_t buf[14];

    if (!data) return 0;

    MPU6050_I2C_ReadMulti(MPU6050_REG_ACCEL_XOUT_H, buf, 14);

    data->accel_x = (int16_t)((buf[0]  << 8) | buf[1]);
    data->accel_y = (int16_t)((buf[2]  << 8) | buf[3]);
    data->accel_z = (int16_t)((buf[4]  << 8) | buf[5]);
    data->temp    = (int16_t)((buf[6]  << 8) | buf[7]);
    data->gyro_x  = (int16_t)((buf[8]  << 8) | buf[9]);
    data->gyro_y  = (int16_t)((buf[10] << 8) | buf[11]);
    data->gyro_z  = (int16_t)((buf[12] << 8) | buf[13]);

    return 1;
}

/**
 * @brief   计算姿态角（互补滤波简化版）
 * @note    基于加速度计计算倾角，仅用于跌倒检测，不做高精度姿态解算
 */
void MPU6050_CalcAttitude(const MPU6050_Data *raw, MPU6050_Attitude *att)
{
    float ax, ay, az;

    if (!raw || !att) return;

    ax = raw->accel_x;
    ay = raw->accel_y;
    az = raw->accel_z;

    /* 俯仰角 (绕Y轴) */
    att->pitch = atan2f(-ax, sqrtf(ay * ay + az * az)) * 180.0f / 3.14159265f;

    /* 翻滚角 (绕X轴) */
    att->roll  = atan2f(ay, sqrtf(ax * ax + az * az)) * 180.0f / 3.14159265f;

    /* 加速度合成值 (mg) */
    att->accel_mag = sqrtf(ax * ax + ay * ay + az * az) / ACCEL_SENSITIVITY * 1000.0f;
}
