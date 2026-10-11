#include "main.h"
#include "bsp_w25q.h"

extern SPI_HandleTypeDef hspi1;   /* ???,?? hspi1 ??? */

#define CS_LOW()  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_RESET)
#define CS_HIGH() HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_SET)

#define CMD_WREN    0x06
#define CMD_SERASE  0x20   /* 4KB ???? */
#define CMD_PAGE    0x02   /* ??,?? 256 ??/? */
#define CMD_READ    0x03
#define CMD_RDSR    0x05   /* ?????? bit0 = BUSY */
#define CMD_RDID    0x9F   /* JEDEC ID,????? */

void w25q_write_enable(void) {
  uint8_t c = CMD_WREN; CS_LOW(); HAL_SPI_Transmit(&hspi1, &c, 1, 100); CS_HIGH();
}

static void w25q_wait_busy(void) {
  uint8_t sr;
  do {
    sr = 0; CS_LOW();
    HAL_SPI_Transmit(&hspi1, (uint8_t[]){CMD_RDSR}, 1, 100);
    HAL_SPI_Receive(&hspi1, &sr, 1, 100); CS_HIGH();
  } while (sr & 0x01);   /* bit0=1 ????? */
}

void w25q_sector_erase(uint32_t addr) {
  uint8_t c[4] = {CMD_SERASE, addr>>16, addr>>8, addr};
  w25q_write_enable(); CS_LOW();
  HAL_SPI_Transmit(&hspi1, c, 4, 100); CS_HIGH();
  w25q_wait_busy();
}

/* ?????(??????,??? 256 ????) */
void w25q_write(uint32_t addr, uint8_t *buf, uint16_t len) {
  while (len > 0) {
    uint16_t in_page = 256 - (addr & 0xFF);   /* ???? */
    uint16_t n = (len < in_page) ? len : in_page;
    uint8_t c[4] = {CMD_PAGE, addr>>16, addr>>8, addr};
    w25q_write_enable(); CS_LOW();
    HAL_SPI_Transmit(&hspi1, c, 4, 100);
    HAL_SPI_Transmit(&hspi1, buf, n, 100); CS_HIGH();
    w25q_wait_busy();
    addr += n; buf += n; len -= n;
  }
}

void w25q_read(uint32_t addr, uint8_t *buf, uint16_t len) {
  uint8_t c[4] = {CMD_READ, addr>>16, addr>>8, addr};
  CS_LOW(); HAL_SPI_Transmit(&hspi1, c, 4, 100);
  HAL_SPI_Receive(&hspi1, buf, len, 100); CS_HIGH();
}

uint16_t w25q_read_id(void) {   /* ? JEDEC ID ?????? */
  uint8_t d[3] = {0}; uint8_t c = CMD_RDID;
  CS_LOW(); HAL_SPI_Transmit(&hspi1, &c, 1, 100);
  HAL_SPI_Receive(&hspi1, d, 3, 100); CS_HIGH();
  return (d[1] << 8) | d[2];    /* W25Q32 ?? 0x4016,W25Q64 ?? 0x4017 */
}