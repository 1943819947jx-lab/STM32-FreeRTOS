#ifndef __BSP_W25Q_H
#define __BSP_W25Q_H
#ifdef __cplusplus
extern "C" {
#endif
#include <stdint.h>
void     w25q_write_enable(void);
void     w25q_sector_erase(uint32_t addr);
void     w25q_write(uint32_t addr, uint8_t *buf, uint16_t len);
void     w25q_read(uint32_t addr, uint8_t *buf, uint16_t len);
uint16_t w25q_read_id(void);
#ifdef __cplusplus
}
#endif
#endif
