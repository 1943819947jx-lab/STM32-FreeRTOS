#ifndef __BSP_OLED_H
#define __BSP_OLED_H
#ifdef __cplusplus
extern "C" {
#endif
#include <stdint.h>

void oled_init(void);
void oled_clear(void);
void oled_string(uint8_t row, char *str);

#ifdef __cplusplus
}
#endif
#endif
