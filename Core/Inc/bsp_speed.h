#ifndef __BSP_SPEED_H
#define __BSP_SPEED_H
#ifdef __cplusplus
extern "C" {
#endif
#include <stdint.h>

/* ??:???????,?? RPM(?/?) */
extern volatile float speed_rpm;

void speed_init(void);   /* ?? TIM2 ??????,main ???? */

#ifdef __cplusplus
}
#endif
#endif
