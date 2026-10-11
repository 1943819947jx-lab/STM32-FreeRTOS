#include "main.h"
#include "bsp_sensor.h"
#include <math.h>

extern ADC_HandleTypeDef hadc1;

#define NTC_B     3950.0f   /* NTC B value, see seller datasheet */
#define NTC_R0    10000.0f  /* 10k @ 25C */
#define NTC_T0    298.15f   /* 25C in Kelvin */
#define SERIES_R  10000.0f  /* divider resistor 10k */
#define AVG_N     10        /* moving average window */

static uint16_t temp_buf[AVG_N] = {0}, light_buf[AVG_N] = {0};

/* moving average: ring buffer + running sum, O(1) update */
static uint16_t avg_filter(uint16_t *buf, uint16_t v) {
  static uint32_t sum = 0;
  static uint8_t i = 0;
  sum += v - buf[i];
  buf[i] = v;
  i = (i + 1) % AVG_N;
  return (uint16_t)(sum / AVG_N);
}

/* read two ADC channels (CubeMX: Rank1=CH0, Rank2=CH1) */
static void read_adc(uint16_t *ch0, uint16_t *ch1) {
  HAL_ADC_Start(&hadc1);
  HAL_ADC_PollForConversion(&hadc1, 10); *ch0 = HAL_ADC_GetValue(&hadc1);
  HAL_ADC_PollForConversion(&hadc1, 10); *ch1 = HAL_ADC_GetValue(&hadc1);
}

float read_temp(void) {
  uint16_t a0, a1;
  read_adc(&a0, &a1);
  uint16_t adc = avg_filter(temp_buf, a0);
  float v = adc * 3.3f / 4095.0f;
  float r = SERIES_R * v / (3.3f - v);   /* derive NTC resistance */
  return 1.0f / (1.0f / NTC_T0 + logf(r / NTC_R0) / NTC_B) - 273.15f;
}

uint8_t read_light(void) {               /* returns 0~100 */
  uint16_t a0, a1;
  read_adc(&a0, &a1);
  return (uint8_t)(avg_filter(light_buf, a1) * 100 / 4095);
}
