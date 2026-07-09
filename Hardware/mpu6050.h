/**
 * @file    mpu6050.h
 * @brief   MPU6050 六轴传感器 — I2C2 驱动
 */
#ifndef __MPU6050_H
#define __MPU6050_H

#include "guide_cane_config.h"

/* MPU6050 数据结构 */
typedef struct {
    int16_t accel_x;
    int16_t accel_y;
    int16_t accel_z;
    int16_t gyro_x;
    int16_t gyro_y;
    int16_t gyro_z;
    int16_t temp;
} MPU6050_Data;

typedef struct {
    float pitch;    /* 俯仰角 (度) */
    float roll;     /* 翻滚角 (度) */
    float accel_mag; /* 加速度合成值 (mg) */
} MPU6050_Attitude;

/* API */
uint8_t MPU6050_Init(void);
uint8_t MPU6050_ReadAll(MPU6050_Data *data);
void    MPU6050_CalcAttitude(const MPU6050_Data *raw, MPU6050_Attitude *att);
uint8_t MPU6050_WhoAmI(void);

#endif /* __MPU6050_H */
