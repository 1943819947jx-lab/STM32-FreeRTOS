#ifndef __BSP_PWM_H
#define __BSP_PWM_H
#ifdef __cplusplus
extern "C" {
#endif
#include <stdint.h>
void buzzer_on(void);
void buzzer_off(void);
void servo_set(uint8_t deg);
void motor_set(int32_t duty);
#ifdef __cplusplus
}
#endif
#endif
