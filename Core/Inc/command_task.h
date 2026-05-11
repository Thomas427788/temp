#ifndef COMMAND_TASK_H
#define COMMAND_TASK_H

#include <stdint.h>

void StartCommandTask(void *argument);
void CDC_ReceiveCallback(uint8_t *buf, uint32_t len);  // ADD THIS

#endif
