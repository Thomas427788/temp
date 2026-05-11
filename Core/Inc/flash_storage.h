#ifndef FLASH_STORAGE_H
#define FLASH_STORAGE_H

#include <stdint.h>

// Magic number so we can detect uninitialised flash
#define FLASH_CAL_MAGIC  0xCAFEBEEF

typedef struct {
    uint32_t magic;
    int32_t  zero_offset;
    float    scale_factor;
} CalibrationData;

uint8_t Flash_LoadCalibration(CalibrationData *out);
void    Flash_SaveCalibration(const CalibrationData *data);

#endif
