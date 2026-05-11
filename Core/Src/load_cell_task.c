#include "main.h"
#include "cmsis_os.h"
#include "hx711.h"
#include "shared_state.h"
#include "flash_storage.h"
#include <stdio.h>
#include <stdint.h>

// Constants
#define CONTACT_AREA_M2     (0.0001f)        // 1cm x 1cm = 1e-4 m²
#define GRAVITY             (9.81f)
#define PASCAL_TO_PSI       (0.000145038f)
#define GRAMS_TO_KG         (0.001f)
#define THRESHOLD_PSI       (1.0f)

volatile uint8_t g_threshold_exceeded = 0;
volatile float g_target_psi = 0.0f;
volatile float g_current_psi = 0.0f;

// Runs the two-point calibration procedure and returns the results
static void run_calibration(int32_t *zero_offset_out, float *scale_factor_out)
{
    printf("=== Load Cell Calibration ===\r\n");
    printf("Remove all weight. Waiting 5 seconds...\r\n");
    osDelay(5000);

    int32_t sum = 0;
    for (int i = 0; i < 10; i++) {
        sum += HX711_ReadRaw();
        osDelay(100);
    }
    *zero_offset_out = sum / 10;
    printf("Zero offset: %ld\r\n", *zero_offset_out);

    // 16g over 1cm² = F/A in PSI
    float cal_force   = 0.016f * GRAVITY;
    float cal_psi     = (cal_force / CONTACT_AREA_M2) * PASCAL_TO_PSI;
    printf("Place known mass (16g -> %.4f PSI over 1cm²). Waiting 10 seconds...\r\n", cal_psi);
    osDelay(10000);

    const float known_weight_g = 16.0f;  // <-- change to your calibration weight
    sum = 0;
    for (int i = 0; i < 10; i++) {
        sum += HX711_ReadRaw();
        osDelay(100);
    }
    int32_t cal_raw = sum / 10;

    if ((cal_raw - *zero_offset_out) != 0)
        *scale_factor_out = (float)(cal_raw - *zero_offset_out) / known_weight_g;
    else
        *scale_factor_out = 1.0f;

    printf("Scale factor: %.2f counts/g\r\n", *scale_factor_out);
}

void StartLoadCellTask(void *argument)
{
    HX711_Init();
    osDelay(500);

    int32_t zero_offset  = 0;
    float   scale_factor = 1.0f;

    // ── Try to load calibration from flash ──────────────────────────────
    CalibrationData cal;
    if (Flash_LoadCalibration(&cal))
    {
        zero_offset  = cal.zero_offset;
        scale_factor = cal.scale_factor;
        printf("Calibration loaded from flash. Offset:%ld Factor:%.2f\r\n",
               zero_offset, scale_factor);
    }
    else
    {
        printf("No calibration found. Send [CALIBRATE] to calibrate.\r\n");
    }

    // ── Measurement loop ─────────────────────────────────────────────────
    uint32_t last_print_ms = 0;

    for (;;)
    {
        // Check if a calibration was requested
        uint32_t flags = osEventFlagsGet(g_cmd_event);
        if (flags & CMD_CALIBRATE_FLAG)
        {
            osEventFlagsClear(g_cmd_event, CMD_CALIBRATE_FLAG);

            run_calibration(&zero_offset, &scale_factor);

            // Save to flash
            CalibrationData new_cal = {
                .magic       = FLASH_CAL_MAGIC,
                .zero_offset = zero_offset,
                .scale_factor = scale_factor
            };
            Flash_SaveCalibration(&new_cal);
            printf("Calibration saved to flash.\r\n");
        }

        // Skip measurement if not yet calibrated
        if (scale_factor == 1.0f && zero_offset == 0)
        {
            osDelay(200);
            continue;
        }

        int32_t raw = HX711_ReadRaw();
            if (raw == INT32_MIN)
            {
                printf("Bad reading skipped\r\n");
                osDelay(50);
                continue;
            }

            float weight_g     = (float)(raw - zero_offset) / scale_factor;
            float force_n      = weight_g * GRAMS_TO_KG * GRAVITY;
            float pressure_pa  = force_n / CONTACT_AREA_M2;
            float pressure_psi = pressure_pa * PASCAL_TO_PSI;

            g_current_psi        = pressure_psi;
            g_threshold_exceeded = (g_target_psi > 0.0f && pressure_psi >= g_target_psi) ? 1 : 0;

            // Only print once per second regardless of sample rate
            uint32_t now = osKernelGetTickCount();
            if (now - last_print_ms >= 1000)
            {
                printf("Raw: %ld  |  Pressure: %.4f PSI\r\n", raw, pressure_psi);
                last_print_ms = now;
            }

            osDelay(g_target_psi > 0.0f ? 50 : 500);
    }
}
