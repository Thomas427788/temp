#ifndef SHARED_STATE_H
#define SHARED_STATE_H

#include <stdint.h>
#include "cmsis_os.h"

typedef struct {
    char    name[64];
    float   pressure;
    float   duration_s;
} CommandParams;

extern volatile uint8_t    g_threshold_exceeded;
extern volatile uint8_t    g_command_received;
extern volatile float g_target_psi;
extern volatile float g_current_psi;
extern CommandParams        g_command_params;
extern osEventFlagsId_t    g_cmd_event;

#define CMD_READY_FLAG       0x00000001U
#define CMD_CALIBRATE_FLAG   0x00000002U   // <-- new

#endif
