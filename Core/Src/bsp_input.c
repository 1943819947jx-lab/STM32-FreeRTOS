#include "main.h"
#include "bsp_input.h"

volatile int32_t target_speed = 0;
volatile uint8_t  param_dirty = 0;
volatile uint8_t  edit_mode   = 0;
volatile uint8_t  mate_state  = 0;

extern float temp_thr;   /* defined in app.c; editable here in edit_mode */

/* EC11 rotary encoder.
   edit_mode == 0: rotate adjusts target_speed (+/-50 per click)
   edit_mode == 1: rotate adjusts temp_thr (+/-1.0 C per click)
   button press toggles edit_mode. */
void HAL_GPIO_EXTI_Callback(uint16_t pin) {
  static uint8_t lastA = 1;
  if (pin == EC11_A_Pin) {
    uint8_t A = HAL_GPIO_ReadPin(EC11_A_GPIO_Port, EC11_A_Pin);
    uint8_t B = HAL_GPIO_ReadPin(EC11_B_GPIO_Port, EC11_B_Pin);
    if (A != lastA) {
      if (edit_mode) {
        if (A == B) temp_thr += 1.0f;
        else        temp_thr -= 1.0f;
        if (temp_thr < 20.0f) temp_thr = 20.0f;
        if (temp_thr > 80.0f) temp_thr = 80.0f;
      } else {
        if (A == B) target_speed += 50;
        else        target_speed -= 50;
        if (target_speed < 0) target_speed = 0;
      }
      lastA = A;
      param_dirty = 1;
    }
  } else if (pin == EC11_KEY_Pin) {
    if (HAL_GPIO_ReadPin(EC11_KEY_GPIO_Port, EC11_KEY_Pin) == 0) edit_mode ^= 1;
  } else if (pin == MATE1_Pin || pin == MATE2_Pin) {
    mate_state = HAL_GPIO_ReadPin(GPIOA, pin);
  }
}
