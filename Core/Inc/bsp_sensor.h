#ifndef __BSP_SENSOR_H
#define __BSP_SENSOR_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

float   read_temp(void);
uint8_t read_light(void);

#ifdef __cplusplus
}
#endif

#endif /* __BSP_SENSOR_H */
