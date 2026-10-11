#include "main.h"
#include "bsp_sensor.h"
#include <math.h>

extern ADC_HandleTypeDef hadc1;

#define NTC_B     3435.0f   /* NTC B value (user module) */
#define NTC_R0    10000.0f  /* 10k @ 25C */
#define NTC_T0    298.15f   /* 25C in Kelvin */
#define SERIES_R  10000.0f  /* divider resistor 10k */
#define AVG_N     10        /* moving average window */

static uint16_t temp_buf[AVG_N] = {0}, light_buf[AVG_N] = {0};
static uint32_t temp_sum = 0, light_sum = 0;
static uint8_t  temp_idx = 0, light_idx = 0;

/* moving average: ring buffer + running sum, O(1) update.
   Each channel has its OWN state -- they must NOT share a static sum. */
static uint16_t avg_filter(uint16_t *buf, uint16_t v, uint32_t *sum, uint8_t *idx) {
  *sum += v - buf[*idx];
  buf[*idx] = v;
  *idx = (*idx + 1) % AVG_N;
  return (uint16_t)(*sum / AVG_N);
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
  uint16_t adc = avg_filter(temp_buf, a0, &temp_sum, &temp_idx);
  float v = adc * 3.3f / 4095.0f;
  float r = SERIES_R * v / (3.3f - v);   /* derive NTC resistance */
  return 1.0f / (1.0f / NTC_T0 + logf(r / NTC_R0) / NTC_B) - 273.15f;
}

uint8_t read_light(void) {               /* returns 0~100 */
  uint16_t a0, a1;
  read_adc(&a0, &a1);
  uint16_t adc = avg_filter(light_buf, a1, &light_sum, &light_idx);
  return (uint8_t)(adc * 100 / 4095);
}
