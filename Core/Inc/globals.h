#ifndef __GLOBALS_H
#define __GLOBALS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "cmsis_os.h"
#include "bsp_speed.h"   /* speed_rpm */
#include "bsp_input.h"   /* target_speed, mate_state, param_dirty, edit_mode */

/* ??????(????) */
typedef struct {
  float temp, pitch, roll, speed;
  uint8_t light, mate;
} SensorData_t;

/* PID ??? */
typedef struct { float Kp, Ki, Kd, integral, prev_err; } PID_t;

/* ???? */
extern osMessageQueueId_t dispQ, alarmQ;

/* PID ????? */
extern PID_t pid;
float pid_calc(PID_t *p, float set, float act);

/* ???? */
extern float temp_thr;

/* Modbus ?? */
extern uint16_t regs[5];
extern uint8_t rx_byte, modbus_rx[64];
extern volatile uint16_t rx_len;
extern volatile uint32_t last_byte_tick;
uint16_t crc16(uint8_t *buf, uint16_t len);
void modbus_handle(uint8_t *rx, uint16_t len);

/* ?????? */
void param_save(void);
void param_load(void);

/* ?????? */
void app_init(void);
void SensorTask(void *arg);
void MotorPIDTask(void *arg);
void DisplayTask(void *arg);
void AlarmActTask(void *arg);
void ParamSaveTask(void *arg);
void ModbusTask(void *arg);

#ifdef __cplusplus
}
#endif

#endif /* __GLOBALS_H */
