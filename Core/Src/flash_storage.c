#include "flash_storage.h"
#include "stm32f4xx_hal.h"
#include <string.h>

// STM32F407 flash sectors:
//   Sector 0  0x08000000  16 KB   <- your code starts here
//   Sector 1  0x08004000  16 KB
//   Sector 2  0x08008000  16 KB
//   Sector 3  0x0800C000  16 KB
//   Sector 4  0x08010000  64 KB
//   Sector 5  0x08020000  128 KB  <- we use this for calibration
//   Sector 6  0x08040000  128 KB
//   Sector 7  0x08060000  128 KB
//
// Adjust FLASH_CAL_SECTOR / FLASH_CAL_ADDR if your code grows into sector 5.

#define FLASH_CAL_SECTOR   FLASH_SECTOR_5
#define FLASH_CAL_ADDR     ((uint32_t)0x08020000)

uint8_t Flash_LoadCalibration(CalibrationData *out)
{
    memcpy(out, (const void *)FLASH_CAL_ADDR, sizeof(CalibrationData));
    return (out->magic == FLASH_CAL_MAGIC);
}

void Flash_SaveCalibration(const CalibrationData *data)
{
    HAL_FLASH_Unlock();

    // Erase the sector first
    FLASH_EraseInitTypeDef erase = {
        .TypeErase    = FLASH_TYPEERASE_SECTORS,
        .VoltageRange = FLASH_VOLTAGE_RANGE_3,   // 2.7–3.6 V supply
        .Sector       = FLASH_CAL_SECTOR,
        .NbSectors    = 1
    };
    uint32_t sector_error = 0;
    HAL_FLASHEx_Erase(&erase, &sector_error);

    // Write word by word
    const uint32_t *src   = (const uint32_t *)data;
    uint32_t        words = (sizeof(CalibrationData) + 3) / 4;
    for (uint32_t i = 0; i < words; i++)
        HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD,
                          FLASH_CAL_ADDR + i * 4,
                          src[i]);

    HAL_FLASH_Lock();
}
