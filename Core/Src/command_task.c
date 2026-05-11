#include "main.h"
#include "cmsis_os.h"
#include "usbd_cdc_if.h"
#include "shared_state.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// ── RX buffer (filled by CDC_ReceiveCallback) ─────────────────────────────
#define RX_BUF_SIZE 128
static char     line_buf[RX_BUF_SIZE];
static uint8_t  rx_index = 0;
volatile uint8_t line_ready = 0;

void CDC_ReceiveCallback(uint8_t *buf, uint32_t len)
{
    for (uint32_t i = 0; i < len; i++) {
        char c = (char)buf[i];
        if (c == '\n' || c == '\r') {
            if (rx_index > 0) {
                line_buf[rx_index] = '\0';
                line_ready = 1;
                rx_index = 0;
            }
        } else if (rx_index < RX_BUF_SIZE - 1) {
            line_buf[rx_index++] = c;
        }
    }
}

// ── Parser ────────────────────────────────────────────────────────────────
// Format: NAME:Test;PRESSURE:30;DURATION:10
// All fields except NAME and PRESSURE are optional.
// To add a new field: add it to CommandParams, then add one strstr block below.
static uint8_t parse_command(const char *cmd, CommandParams *out)
{
    memset(out, 0, sizeof(CommandParams));
    out->duration_s = 0.0f;  // default: run forever

    // NAME (required)
    const char *p = strstr(cmd, "NAME:");
    if (!p) return 0;
    p += 5;
    const char *end = strchr(p, ';');
    size_t len = end ? (size_t)(end - p) : strlen(p);
    if (len == 0 || len >= sizeof(out->name)) return 0;
    strncpy(out->name, p, len);

    // PRESSURE (required)
    p = strstr(cmd, "PRESSURE:");
    if (!p) return 0;
    out->pressure = strtof(p + 9, NULL);

    // DURATION (optional) — e.g. DURATION:10
    // DURATION / TIME — accept either field name
    p = strstr(cmd, "DURATION:");
    if (p) {
        out->duration_s = strtof(p + 9, NULL);
    } else {
        p = strstr(cmd, "TIME:");
        if (p) out->duration_s = strtof(p + 5, NULL);
    }

    // ── ADD NEW OPTIONAL FIELDS HERE ──────────────────────────────────────
    // p = strstr(cmd, "SPEED:");
    // if (p) out->speed = strtof(p + 6, NULL);
    // ─────────────────────────────────────────────────────────────────────

    return 1;
}

// ── Task ──────────────────────────────────────────────────────────────────
void StartCommandTask(void *argument)
{
    osDelay(1500);  // wait for USB CDC to enumerate
    printf("STM32 ready. Waiting for command...\r\n");
    printf("Format: NAME:Test;PRESSURE:30;DURATION:10\r\n");

    for (;;)
    {
        if (line_ready)
        {
        	line_ready = 0;
        	CommandParams params;   // <-- add this line here
        	// Inside the if (line_ready) block, BEFORE parse_command:

        	if (strcmp(line_buf, "CMD:CALIBRATE") == 0)
        	{
        	    printf("CALIBRATE command received\r\n");
        	    osEventFlagsSet(g_cmd_event, CMD_CALIBRATE_FLAG);
        	}
        	else if (parse_command(line_buf, &params))
        	{
        	    taskENTER_CRITICAL();
        	    g_command_params   = params;
        	    g_command_received = 1;
        	    taskEXIT_CRITICAL();

        	    printf("ACK:NAME:%s;PRESSURE:%.2f;DURATION:%.1f;STATUS:OK\r\n",
        	           params.name, params.pressure, params.duration_s);

        	    osEventFlagsSet(g_cmd_event, CMD_READY_FLAG);
        	}
        	else
        	{
        	    printf("ERR:BAD_COMMAND\r\n");
        	}
        }
        osDelay(10);
    }
}
