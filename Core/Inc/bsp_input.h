#ifndef __BSP_INPUT_H
#define __BSP_INPUT_H
#ifdef __cplusplus
extern "C" {
#endif
#include <stdint.h>

extern volatile int32_t target_speed;   /* EC11 ??????? */
extern volatile uint8_t  param_dirty;   /* ????,??? */
extern volatile uint8_t  edit_mode;     /* ??????? */
extern volatile uint8_t  mate_state;    /* ?????? */

#ifdef __cplusplus
}
#endif
#endif
