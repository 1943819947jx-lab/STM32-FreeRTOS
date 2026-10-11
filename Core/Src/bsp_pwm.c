#include "main.h"
#include "bsp_pwm.h"

extern TIM_HandleTypeDef htim1;   /* ??? */
extern TIM_HandleTypeDef htim4;   /* ?? */
extern TIM_HandleTypeDef htim3;   /* ?? */


void buzzer_on(void)  { __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, 0);   HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1); } /* PA8 low  -> beep */
void buzzer_off(void) { __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, 500); HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1); } /* PA8 high -> mute */

/* ?? 0~180�:CCR 500~2500 ?? 0.5~2.5ms(?? 20ms) */
void servo_set(uint8_t deg) {
  if (deg > 180) deg = 180;
  uint32_t ccr = 500 + deg * (2500 - 500) / 180;
  __HAL_TIM_SET_COMPARE(&htim4, TIM_CHANNEL_3, ccr);
  HAL_TIM_PWM_Start(&htim4, TIM_CHANNEL_3);
}

/* ??:duty �10000 ?? �100%,???? */
void motor_set(int32_t duty) {
  if (duty >= 0) {
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, 1);   /* IN1=1, IN2=0 ?? */
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_13, 0);
  } else {
    duty = -duty;
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, 0);   /* IN1=0, IN2=1 ?? */
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_13, 1);
  }
  if (duty > 10000) duty = 10000;
  __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_3, duty * 7199 / 10000);
  HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_3);
}