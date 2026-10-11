#include "main.h"
#include "bsp_mpu6050.h"
#include <math.h>

extern I2C_HandleTypeDef hi2c1;

#define MPU_ADDR (0x68u << 1)   /* AD0 -> GND: addr 0x68, shifted for 7-bit write */

static float ang_x = 0, ang_y = 0;      /* low-pass filtered angle */

static void mpu_write(uint8_t reg, uint8_t val) {
  uint8_t d[2] = {reg, val};
  HAL_I2C_Master_Transmit(&hi2c1, MPU_ADDR, d, 2, 100);
}

static void mpu_read(uint8_t reg, uint8_t *buf, uint8_t len) {
  HAL_I2C_Master_Transmit(&hi2c1, MPU_ADDR, &reg, 1, 100);
  HAL_I2C_Master_Receive(&hi2c1, MPU_ADDR, buf, len, 100);
}

void mpu_init(void) {
  mpu_write(0x6B, 0x00);   /* PWR_MGMT_1: wake up */
  mpu_write(0x1B, 0x00);   /* GYRO_CONFIG: +-250dps -> 131 LSB/(deg/s) */
  mpu_write(0x1C, 0x00);   /* ACCEL_CONFIG: +-2g -> 16384 LSB/g */
}

void mpu_get_angle(float *pitch, float *roll) {
  uint8_t b[6]; int16_t ax, ay, az;
  mpu_read(0x3B, b, 6);   /* AccX AccY AccZ */
  ax = (b[0]<<8)|b[1]; ay = (b[2]<<8)|b[3]; az = (b[4]<<8)|b[5];

  /* accelerometer-only tilt: bounded +-180, no drift */
  float acc_roll  = atan2f(ay, az) * 57.2958f;
  float acc_pitch = atan2f(-ax, sqrtf(ay*ay + az*az)) * 57.2958f;

  /* light low-pass to smooth vibration */
  ang_x = 0.9f * ang_x + 0.1f * acc_roll;
  ang_y = 0.9f * ang_y + 0.1f * acc_pitch;
  *pitch = ang_x; *roll = ang_y;
}
