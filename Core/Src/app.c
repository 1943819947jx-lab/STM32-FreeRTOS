#include "main.h"
#include "globals.h"
#include "bsp_sensor.h"
#include "bsp_mpu6050.h"
#include "bsp_pwm.h"
#include "bsp_oled.h"
#include "bsp_w25q.h"
#include "usart.h"
#include <stdio.h>

/* ================= ???? ================= */
osMessageQueueId_t dispQ = NULL, alarmQ = NULL;
PID_t pid = {.Kp = 2.0f, .Ki = 0.1f, .Kd = 0.05f};
float temp_thr = 35.0f;

uint16_t regs[5] = {0};
uint8_t rx_byte = 0, modbus_rx[64] = {0};
volatile uint16_t rx_len = 0;
volatile uint32_t last_byte_tick = 0;
volatile uint8_t  alarm_active = 0;

/* ================= PID ================= */
float pid_calc(PID_t *p, float set, float act) {
  float err = set - act;
  p->integral += err;
  if (p->integral > 8000)  p->integral = 8000;   /* ???? */
  if (p->integral < -8000) p->integral = -8000;
  float out = p->Kp * err + p->Ki * p->integral + p->Kd * (err - p->prev_err);
  p->prev_err = err;
  if (out > 10000)  out = 10000;                  /* ???? */
  if (out < -10000) out = -10000;
  return out;
}

/* ================= ????(W25Q) ================= */
typedef struct { uint32_t magic; float target_speed; float temp_thr; } Param_t;
#define PARAM_ADDR 0x0000
#define PARAM_MAGIC 0xA5A5A5A5

void param_save(void) {
  Param_t p = {PARAM_MAGIC, (float)target_speed, temp_thr};
  w25q_sector_erase(PARAM_ADDR);
  w25q_write(PARAM_ADDR, (uint8_t *)&p, sizeof(Param_t));
}

void param_load(void) {
  Param_t p;
  w25q_read(PARAM_ADDR, (uint8_t *)&p, sizeof(Param_t));
  if (p.magic == PARAM_MAGIC) {          /* ??????,?????? */
    target_speed = (int32_t)p.target_speed;
    temp_thr = p.temp_thr;
  }
}

/* ================= Modbus ================= */
uint16_t crc16(uint8_t *buf, uint16_t len) {
  uint16_t crc = 0xFFFF;
  for (uint16_t i = 0; i < len; i++) {
    crc ^= buf[i];
    for (int j = 0; j < 8; j++)
      crc = (crc & 1) ? (crc >> 1) ^ 0xA001 : crc >> 1;
  }
  return crc;
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
  if (huart->Instance == USART1) {
    modbus_rx[rx_len++] = rx_byte;
    last_byte_tick = HAL_GetTick();
    HAL_UART_Receive_IT(&huart1, &rx_byte, 1);   /* ????????? */
  }
}

void modbus_handle(uint8_t *rx, uint16_t len) {
  if (crc16(rx, len - 2) != (rx[len-2] | (rx[len-1] << 8))) return;
  if (rx[0] != 0x01) return;                     /* ????????? */

  uint8_t tx[64]; uint16_t tl = 0;
  if (rx[1] == 0x03) {                           /* ?????? */
    uint16_t start = (rx[2]<<8) | rx[3], num = (rx[4]<<8) | rx[5];
    tx[tl++] = 0x01; tx[tl++] = 0x03; tx[tl++] = num * 2;
    for (uint16_t i = 0; i < num; i++) {
      uint16_t v = (start + i < 5) ? regs[start + i] : 0;
      tx[tl++] = v >> 8; tx[tl++] = v & 0xFF;
    }
  } else if (rx[1] == 0x06) {                    /* ????? */
    uint16_t addr = (rx[2]<<8) | rx[3], val = (rx[4]<<8) | rx[5];
    if (addr < 5) { regs[addr] = val; param_dirty = 1; }
    for (uint16_t i = 0; i < len; i++) tx[tl++] = rx[i];
  } else return;

  uint16_t c = crc16(tx, tl);
  tx[tl++] = c & 0xFF; tx[tl++] = c >> 8;
  HAL_UART_Transmit(&huart1, tx, tl, 100);
}

/* ================= ?? ================= */
/* MicroLIB ? sprintf ??? %f,??????????? */
static void fmt1(char *s, float v) {
  int n = (int)(v * 10.0f + (v >= 0.0f ? 0.5f : -0.5f));
  int neg = (n < 0);
  if (neg) n = -n;
  int a = n / 10, b = n % 10;
  if (neg) *s++ = '-';
  sprintf(s, "%d.%d", a, b);
}

void SensorTask(void *arg) {                     /* 200ms ??,???? */
  for (;;) {
    SensorData_t d;
    d.temp  = read_temp();
    d.light = read_light();
    mpu_get_angle(&d.pitch, &d.roll);
    d.speed = speed_rpm;
    /* TCRT5000: 有料(反射强)->DO低->mate=1(不报警); 缺料->DO高->mate=0(报警) */
    d.mate  = (HAL_GPIO_ReadPin(MATE1_GPIO_Port, MATE1_Pin) == GPIO_PIN_RESET) ? 1 : 0;
    regs[0] = (uint16_t)(d.temp * 10);
    regs[1] = d.light;
    regs[2] = (uint16_t)d.speed;
    regs[3] = (uint16_t)(d.pitch * 10);
    regs[4] = d.mate;
    osMessageQueuePut(dispQ,  &d, 0, 0);
    osMessageQueuePut(alarmQ, &d, 0, 0);
    osDelay(200);
  }
}

void MotorPIDTask(void *arg) {                   /* 10ms PID,?????? */
  for (;;) {
    if (target_speed < 0) target_speed = 0;   /* sensor measures magnitude only */
    float out = pid_calc(&pid, (float)target_speed, speed_rpm);
    motor_set((int32_t)out);
    osDelay(10);
  }
}

void DisplayTask(void *arg) {                    /* ??????? OLED */
  for (;;) {
    SensorData_t d;
    if (osMessageQueueGet(dispQ, &d, NULL, 0) == osOK) {
      char b1[8], buf[24];
      oled_clear();
      fmt1(b1, d.temp);    sprintf(buf, "T:%sC", b1);   oled_string(0, buf);
      sprintf(buf, "%s", alarm_active ? "ALARM!" : "OK"); oled_string(1, buf);
      sprintf(buf, "L:%d%%", d.light);                  oled_string(2, buf);
      sprintf(buf, edit_mode ? ">TH:%d" : " TH:%d", (int)temp_thr); oled_string(3, buf);
      sprintf(buf, "V:%d", (int)d.speed);               oled_string(4, buf);
      sprintf(buf, edit_mode ? " S:%d" : ">S:%d", (int)target_speed);          oled_string(5, buf);
      sprintf(buf, "M:%s", d.mate ? "OK":"NO");         oled_string(6, buf);
      fmt1(b1, d.pitch);   sprintf(buf, "A:%s", b1);    oled_string(7, buf);
    }
    osDelay(50);
  }
}

void AlarmActTask(void *arg) {                   /* 100ms ??,???? */
  for (;;) {
    SensorData_t d;
    if (osMessageQueueGet(alarmQ, &d, NULL, 0) == osOK) {
      uint8_t alarm = 0;
      if (d.temp > temp_thr)                alarm = 1;   /* ???? */
      if (d.pitch > 45 || d.pitch < -45 || d.roll > 45 || d.roll < -45)    alarm = 1;   /* ???? */
      if (d.mate == 0)                        alarm = 1;   /* ???? */

      alarm_active = alarm;
      if (alarm) { buzzer_on(); servo_set(90); }  /* ?? + ???? */
      else       { buzzer_off(); servo_set(0);  } /* ?? + ?? */
    }
    osDelay(100);
  }
}

void ParamSaveTask(void *arg) {                  /* ?????? Flash */
  for (;;) {
    if (param_dirty) { param_save(); param_dirty = 0; }
    osDelay(50);
  }
}

void ModbusTask(void *arg) {                     /* ?? + ?? */
  for (;;) {
    if (rx_len >= 8 && (HAL_GetTick() - last_byte_tick) > 5) {
      modbus_handle(modbus_rx, rx_len);
      rx_len = 0;
    }
    osDelay(10);
  }
}

/* ================= ???? ================= */
static const osThreadAttr_t attr_small = {
  .name = "task", .stack_size = 256 * 4, .priority = osPriorityNormal,
};
static const osThreadAttr_t attr_disp = {
  .name = "disp", .stack_size = 256 * 4, .priority = osPriorityNormal,
};

void app_init(void) {
  dispQ  = osMessageQueueNew(10, sizeof(SensorData_t), NULL);
  alarmQ = osMessageQueueNew(10, sizeof(SensorData_t), NULL);

  osThreadNew(SensorTask,    NULL, &attr_small);
  osThreadNew(MotorPIDTask,  NULL, &attr_small);
  osThreadNew(DisplayTask,   NULL, &attr_disp);
  osThreadNew(AlarmActTask,  NULL, &attr_small);
  osThreadNew(ParamSaveTask, NULL, &attr_small);
  osThreadNew(ModbusTask,    NULL, &attr_small);
}
