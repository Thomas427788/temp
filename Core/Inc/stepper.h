#ifndef STEPPER_H
#define STEPPER_H

#include "main.h"
#include <stdint.h>

void Stepper_Init(void);
void Stepper_Step(int steps, uint16_t delay_us, int direction);

#endif
