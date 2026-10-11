#ifndef __BSP_MPU6050_H
#define __BSP_MPU6050_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

void mpu_init(void);
void mpu_get_angle(float *pitch, float *roll);

#ifdef __cplusplus
}
#endif

#endif /* __BSP_MPU6050_H */
