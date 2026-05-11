#ifndef HX711_H
#define HX711_H

#include "main.h"
#include <stdint.h>

// PD0 = CLK, PD1 = DOUT
#define HX711_CLK_PIN   GPIO_PIN_0
#define HX711_CLK_PORT  GPIOD
#define HX711_DOUT_PIN  GPIO_PIN_1
#define HX711_DOUT_PORT GPIOD

void     HX711_Init(void);
int32_t  HX711_ReadRaw(void);
uint8_t  HX711_IsReady(void);

#endif
